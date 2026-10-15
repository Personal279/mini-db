package server

import (
	"encoding/json"
	"errors"
	"net/http"
	"strings"

	"mini-couchbase/index-engine/index"
)

type Handler struct {
	indexer *index.DocumentIndexer
	mux     *http.ServeMux
}

func NewHandler(indexer *index.DocumentIndexer) *Handler {
	h := &Handler{
		indexer: indexer,
		mux:     http.NewServeMux(),
	}

	h.mux.HandleFunc("/indexes", h.handleIndexes)
	h.mux.HandleFunc("/indexes/", h.handleIndexLookup)
	h.mux.HandleFunc("/documents/", h.handleDocument)

	return h
}

func (h *Handler) ServeHTTP(
	w http.ResponseWriter,
	r *http.Request,
) {
	h.mux.ServeHTTP(w, r)
}

func writeJSON(w http.ResponseWriter, status int, value any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(value)
}

func writeError(w http.ResponseWriter, status int, message string) {
	writeJSON(w, status, map[string]string{"error": message})
}

func (h *Handler) handleIndexes(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path != "/indexes" {
		http.NotFound(w, r)
		return
	}

	switch r.Method {
	case http.MethodGet:
		writeJSON(w, http.StatusOK, h.indexer.ListIndexes())

	case http.MethodPost:
		var request struct {
			Name  string `json:"name"`
			Field string `json:"field"`
		}

		if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
			writeError(w, http.StatusBadRequest, "invalid JSON body")
			return
		}

		if err := h.indexer.CreateIndex(request.Name, request.Field); err != nil {
			if errors.Is(err, index.ErrIndexExists) {
				writeError(w, http.StatusConflict, err.Error())
				return
			}

			writeError(w, http.StatusBadRequest, err.Error())
			return
		}

		writeJSON(w, http.StatusCreated, map[string]string{
			"status": "created",
			"name":   request.Name,
			"field":  request.Field,
		})

	default:
		w.Header().Set("Allow", "GET, POST")
		writeError(w, http.StatusMethodNotAllowed, "method not allowed")
	}
}

func (h *Handler) handleIndexLookup(w http.ResponseWriter, r *http.Request) {
	const prefix = "/indexes/"

	name := strings.TrimPrefix(r.URL.Path, prefix)
	if name == "" || strings.Contains(name, "/") {
		http.NotFound(w, r)
		return
	}

	if r.Method != http.MethodGet {
		w.Header().Set("Allow", http.MethodGet)
		writeError(w, http.StatusMethodNotAllowed, "method not allowed")
		return
	}

	value, exists := r.URL.Query()["value"]
	if !exists || len(value) == 0 {
		writeError(w, http.StatusBadRequest, "missing value query parameter")
		return
	}

	keys, err := h.indexer.Lookup(name, value[0])
	if err != nil {
		writeError(w, http.StatusNotFound, err.Error())
		return
	}

	writeJSON(w, http.StatusOK, map[string]any{
		"index": name,
		"value": value[0],
		"keys":  keys,
	})
}

func (h *Handler) handleDocument(w http.ResponseWriter, r *http.Request) {
	const prefix = "/documents/"

	key := strings.TrimPrefix(r.URL.Path, prefix)
	if key == "" || strings.Contains(key, "/") {
		http.NotFound(w, r)
		return
	}

	switch r.Method {
	case http.MethodPut:
		body, err := readRequestBody(r)
		if err != nil {
			writeError(w, http.StatusBadRequest, err.Error())
			return
		}

		if err := h.indexer.UpsertJSON(key, body); err != nil {
			writeError(w, http.StatusBadRequest, err.Error())
			return
		}

		writeJSON(w, http.StatusOK, map[string]string{
			"status": "indexed",
			"key":    key,
		})

	case http.MethodDelete:
		if err := h.indexer.Delete(key); err != nil {
			writeError(w, http.StatusInternalServerError, err.Error())
			return
		}

		writeJSON(w, http.StatusOK, map[string]string{
			"status": "deleted",
			"key":    key,
		})

	default:
		w.Header().Set("Allow", "PUT, DELETE")
		writeError(w, http.StatusMethodNotAllowed, "method not allowed")
	}
}

func readRequestBody(r *http.Request) ([]byte, error) {
	defer r.Body.Close()

	var body json.RawMessage
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
		return nil, errors.New("invalid JSON document")
	}

	if len(body) == 0 || string(body) == "null" {
		return nil, errors.New("document must be a JSON object")
	}

	return body, nil
}
