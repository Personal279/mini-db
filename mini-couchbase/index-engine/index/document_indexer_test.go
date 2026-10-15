package index

import (
	"errors"
	"reflect"
	"sort"
	"testing"
)

func assertDocumentLookup(
	t *testing.T,
	indexer *DocumentIndexer,
	indexName string,
	value string,
	expected []string,
) {
	t.Helper()

	got, err := indexer.Lookup(indexName, value)
	if err != nil {
		t.Fatalf("Lookup(%q, %q) failed: %v", indexName, value, err)
	}

	sort.Strings(got)
	sort.Strings(expected)

	if !reflect.DeepEqual(got, expected) {
		t.Fatalf(
			"Lookup(%q, %q) = %v; expected %v",
			indexName,
			value,
			got,
			expected,
		)
	}
}

func TestDocumentIndexerInsertAndLookup(t *testing.T) {
	manager := NewIndexManager()
	indexer := NewDocumentIndexer(manager)

	if err := indexer.CreateIndex("idx_city", "city"); err != nil {
		t.Fatal(err)
	}

	if err := indexer.Upsert("user:1", map[string]string{
		"city": "Bengaluru",
	}); err != nil {
		t.Fatal(err)
	}

	if err := indexer.Upsert("user:2", map[string]string{
		"city": "Udupi",
	}); err != nil {
		t.Fatal(err)
	}

	if err := indexer.Upsert("user:3", map[string]string{
		"city": "Bengaluru",
	}); err != nil {
		t.Fatal(err)
	}

	assertDocumentLookup(
		t, indexer, "idx_city", "Bengaluru",
		[]string{"user:1", "user:3"},
	)
	assertDocumentLookup(
		t, indexer, "idx_city", "Udupi",
		[]string{"user:2"},
	)
}

func TestDocumentIndexerUpdate(t *testing.T) {
	manager := NewIndexManager()
	indexer := NewDocumentIndexer(manager)

	if err := indexer.CreateIndex("idx_city", "city"); err != nil {
		t.Fatal(err)
	}

	if err := indexer.Upsert("user:1", map[string]string{
		"city": "Bengaluru",
	}); err != nil {
		t.Fatal(err)
	}

	if err := indexer.Upsert("user:1", map[string]string{
		"city": "Udupi",
	}); err != nil {
		t.Fatal(err)
	}

	assertDocumentLookup(
		t, indexer, "idx_city", "Bengaluru", []string{},
	)
	assertDocumentLookup(
		t, indexer, "idx_city", "Udupi", []string{"user:1"},
	)
}

func TestDocumentIndexerDelete(t *testing.T) {
	manager := NewIndexManager()
	indexer := NewDocumentIndexer(manager)

	if err := indexer.CreateIndex("idx_city", "city"); err != nil {
		t.Fatal(err)
	}

	if err := indexer.Upsert("user:1", map[string]string{
		"city": "Bengaluru",
	}); err != nil {
		t.Fatal(err)
	}

	if err := indexer.Delete("user:1"); err != nil {
		t.Fatal(err)
	}

	assertDocumentLookup(
		t, indexer, "idx_city", "Bengaluru", []string{},
	)
}

func TestDocumentIndexerCreateIndexAfterDocuments(t *testing.T) {
	manager := NewIndexManager()
	indexer := NewDocumentIndexer(manager)

	if err := indexer.Upsert("user:1", map[string]string{
		"city": "Bengaluru",
	}); err != nil {
		t.Fatal(err)
	}

	if err := indexer.CreateIndex("idx_city", "city"); err != nil {
		t.Fatal(err)
	}

	assertDocumentLookup(
		t, indexer, "idx_city", "Bengaluru", []string{"user:1"},
	)
}

func TestDocumentIndexerMissingIndex(t *testing.T) {
	manager := NewIndexManager()
	indexer := NewDocumentIndexer(manager)

	_, err := indexer.Lookup("idx_missing", "Bengaluru")
	if !errors.Is(err, ErrIndexNotFound) {
		t.Fatalf("expected ErrIndexNotFound, got %v", err)
	}
}

func TestDocumentIndexerRejectsEmptyKey(t *testing.T) {
	manager := NewIndexManager()
	indexer := NewDocumentIndexer(manager)

	err := indexer.Upsert("", map[string]string{"city": "Udupi"})
	if !errors.Is(err, ErrEmptyDocumentKey) {
		t.Fatalf("expected ErrEmptyDocumentKey, got %v", err)
	}
}

func TestDocumentIndexerMissingField(t *testing.T) {
	manager := NewIndexManager()
	indexer := NewDocumentIndexer(manager)

	if err := indexer.CreateIndex("idx_city", "city"); err != nil {
		t.Fatal(err)
	}

	if err := indexer.Upsert("user:1", map[string]string{
		"email": "user1@example.com",
	}); err != nil {
		t.Fatal(err)
	}

	assertDocumentLookup(
		t, indexer, "idx_city", "Bengaluru", []string{},
	)
}

func TestUpsertJSONUpdatesIndex(t *testing.T) {
	manager := NewIndexManager()
	indexer := NewDocumentIndexer(manager)

	if err := indexer.CreateIndex("city_idx", "city"); err != nil {
		t.Fatal(err)
	}

	if err := indexer.UpsertJSON(
		"user:101",
		[]byte(`{"name":"Harsha","city":"Udupi"}`),
	); err != nil {
		t.Fatal(err)
	}

	if err := indexer.UpsertJSON(
		"user:101",
		[]byte(`{"name":"Harsha","city":"Bengaluru"}`),
	); err != nil {
		t.Fatal(err)
	}

	oldKeys, err := indexer.Lookup("city_idx", "Udupi")
	if err != nil {
		t.Fatal(err)
	}
	if len(oldKeys) != 0 {
		t.Fatalf("expected no keys for Udupi, got %v", oldKeys)
	}

	newKeys, err := indexer.Lookup("city_idx", "Bengaluru")
	if err != nil {
		t.Fatal(err)
	}
	if len(newKeys) != 1 || newKeys[0] != "user:101" {
		t.Fatalf("expected [user:101] for Bengaluru, got %v", newKeys)
	}
}
