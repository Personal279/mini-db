package index

import (
	"encoding/json"
	"errors"
	"fmt"
	"sync"
)

var ErrEmptyIndexName = errors.New("index name cannot be empty")
var ErrEmptyFieldName = errors.New("indexed field name cannot be empty")
var ErrEmptyDocumentKey = errors.New("document key cannot be empty")

// DocumentIndexer maintains documents and keeps registered secondary
// indexes synchronized with document inserts, updates, and deletes.
type DocumentIndexer struct {
	mu          sync.RWMutex
	manager     *IndexManager
	indexFields map[string]string
	documents   map[string]map[string]string
}

type IndexMetadata struct {
	Name  string `json:"name"`
	Field string `json:"field"`
}

func (d *DocumentIndexer) ListIndexes() []IndexMetadata {
	d.mu.RLock()
	defer d.mu.RUnlock()

	result := make([]IndexMetadata, 0, len(d.indexFields))

	for name, field := range d.indexFields {
		result = append(result, IndexMetadata{
			Name:  name,
			Field: field,
		})
	}

	return result
}

func NewDocumentIndexer(manager *IndexManager) *DocumentIndexer {
	if manager == nil {
		panic("IndexManager cannot be nil")
	}

	return &DocumentIndexer{
		manager:     manager,
		indexFields: make(map[string]string),
		documents:   make(map[string]map[string]string),
	}
}

// CreateIndex creates a secondary index for a document field.
// Existing documents are indexed as well.
func (d *DocumentIndexer) CreateIndex(name, field string) error {
	if name == "" {
		return ErrEmptyIndexName
	}
	if field == "" {
		return ErrEmptyFieldName
	}

	d.mu.Lock()
	defer d.mu.Unlock()

	if _, exists := d.indexFields[name]; exists {
		return ErrIndexExists
	}

	if err := d.manager.CreateIndex(name); err != nil {
		return err
	}

	idx, err := d.manager.GetIndex(name)
	if err != nil {
		_ = d.manager.DropIndex(name)
		return err
	}

	d.indexFields[name] = field

	for documentKey, document := range d.documents {
		if value, exists := document[field]; exists {
			idx.Insert(value, documentKey)
		}
	}

	return nil
}

// Upsert inserts a document or replaces an existing document.
// All registered secondary indexes are updated accordingly.
func (d *DocumentIndexer) Upsert(
	documentKey string,
	document map[string]string,
) error {
	if documentKey == "" {
		return ErrEmptyDocumentKey
	}

	d.mu.Lock()
	defer d.mu.Unlock()

	// Remove the document's previous index entries.
	if previous, exists := d.documents[documentKey]; exists {
		for name, field := range d.indexFields {
			oldValue, hadOldValue := previous[field]
			if !hadOldValue {
				continue
			}

			idx, err := d.manager.GetIndex(name)
			if err != nil {
				return err
			}

			idx.Delete(oldValue, documentKey)
		}
	}

	// Copy the input so callers cannot mutate our stored document
	// by changing their original map.
	copyOfDocument := make(map[string]string, len(document))
	for field, value := range document {
		copyOfDocument[field] = value
	}

	d.documents[documentKey] = copyOfDocument

	// Insert entries for all fields that have registered indexes.
	for name, field := range d.indexFields {
		value, exists := copyOfDocument[field]
		if !exists {
			continue
		}

		idx, err := d.manager.GetIndex(name)
		if err != nil {
			return err
		}

		idx.Insert(value, documentKey)
	}

	return nil
}

// UpsertJSON parses a JSON object containing string-valued fields
// and inserts or replaces the corresponding indexed document.
func (d *DocumentIndexer) UpsertJSON(
	documentKey string,
	rawJSON []byte,
) error {
	if documentKey == "" {
		return ErrEmptyDocumentKey
	}

	var document map[string]string

	if err := json.Unmarshal(rawJSON, &document); err != nil {
		return fmt.Errorf("decode JSON document: %w", err)
	}

	if document == nil {
		return errors.New("JSON document must be an object")
	}

	return d.Upsert(documentKey, document)
}

// Delete removes a document and all its secondary index entries.
// Deleting an unknown document is harmless.
func (d *DocumentIndexer) Delete(documentKey string) error {
	if documentKey == "" {
		return ErrEmptyDocumentKey
	}

	d.mu.Lock()
	defer d.mu.Unlock()

	document, exists := d.documents[documentKey]
	if !exists {
		return nil
	}

	for name, field := range d.indexFields {
		value, hasValue := document[field]
		if !hasValue {
			continue
		}

		idx, err := d.manager.GetIndex(name)
		if err != nil {
			return err
		}

		idx.Delete(value, documentKey)
	}

	delete(d.documents, documentKey)
	return nil
}

// Lookup returns the document keys matching a value in a named index.
func (d *DocumentIndexer) Lookup(
	indexName string,
	value string,
) ([]string, error) {
	d.mu.RLock()
	defer d.mu.RUnlock()

	idx, err := d.manager.GetIndex(indexName)
	if err != nil {
		return nil, err
	}

	return idx.Lookup(value), nil
}
