package main

import (
	"log"
	"net/http"
	"os"
	"time"

	"mini-couchbase/index-engine/index"
	"mini-couchbase/index-engine/server"
)

func main() {
	manager := index.NewIndexManager()
	indexer := index.NewDocumentIndexer(manager)
	handler := server.NewHandler(indexer)

	address := os.Getenv("INDEX_SERVER_ADDR")
	if address == "" {
		address = "127.0.0.1:8090"
	}

	httpServer := &http.Server{
		Addr:              address,
		Handler:           handler,
		ReadHeaderTimeout: 5 * time.Second,
	}

	log.Printf("Index Service listening on http://%s", address)

	if err := httpServer.ListenAndServe(); err != nil &&
		err != http.ErrServerClosed {
		log.Fatal(err)
	}
}
