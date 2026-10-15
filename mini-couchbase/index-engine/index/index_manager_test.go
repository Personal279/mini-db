package index

import (
	"errors"
	"testing"
)

func TestCreateIndex(t *testing.T) {
	manager := NewIndexManager()

	err := manager.CreateIndex("idx_city")
	if err != nil {
		t.Fatalf("CreateIndex failed: %v", err)
	}

	// The newly created index should be retrievable.
	idx, err := manager.GetIndex("idx_city")
	if err != nil {
		t.Fatalf("GetIndex failed: %v", err)
	}

	if idx == nil {
		t.Fatal("GetIndex returned a nil index")
	}
}

func TestCreateDuplicateIndex(t *testing.T) {
	manager := NewIndexManager()

	if err := manager.CreateIndex("idx_city"); err != nil {
		t.Fatalf("first CreateIndex failed: %v", err)
	}

	err := manager.CreateIndex("idx_city")
	if !errors.Is(err, ErrIndexExists) {
		t.Fatalf("expected ErrIndexExists, got %v", err)
	}
}

func TestGetIndexNotFound(t *testing.T) {
	manager := NewIndexManager()

	_, err := manager.GetIndex("idx_missing")
	if !errors.Is(err, ErrIndexNotFound) {
		t.Fatalf("expected ErrIndexNotFound, got %v", err)
	}
}

func TestIndexesAreIndependent(t *testing.T) {
	manager := NewIndexManager()

	if err := manager.CreateIndex("idx_city"); err != nil {
		t.Fatal(err)
	}

	if err := manager.CreateIndex("idx_email"); err != nil {
		t.Fatal(err)
	}

	cityIndex, err := manager.GetIndex("idx_city")
	if err != nil {
		t.Fatal(err)
	}

	emailIndex, err := manager.GetIndex("idx_email")
	if err != nil {
		t.Fatal(err)
	}

	cityIndex.Insert("Bengaluru", "user:1")
	emailIndex.Insert("user@example.com", "user:2")

	if got := cityIndex.Lookup("Bengaluru"); len(got) != 1 || got[0] != "user:1" {
		t.Fatalf("unexpected city index result: %v", got)
	}

	if got := emailIndex.Lookup("user@example.com"); len(got) != 1 || got[0] != "user:2" {
		t.Fatalf("unexpected email index result: %v", got)
	}

	if got := cityIndex.Lookup("user@example.com"); len(got) != 0 {
		t.Fatalf("city index unexpectedly contains email value: %v", got)
	}
}

func TestDropIndex(t *testing.T) {
	manager := NewIndexManager()

	if err := manager.CreateIndex("idx_city"); err != nil {
		t.Fatal(err)
	}

	if err := manager.DropIndex("idx_city"); err != nil {
		t.Fatalf("DropIndex failed: %v", err)
	}

	_, err := manager.GetIndex("idx_city")
	if !errors.Is(err, ErrIndexNotFound) {
		t.Fatalf("expected ErrIndexNotFound after drop, got %v", err)
	}
}

func TestDropIndexNotFound(t *testing.T) {
	manager := NewIndexManager()

	err := manager.DropIndex("idx_missing")
	if !errors.Is(err, ErrIndexNotFound) {
		t.Fatalf("expected ErrIndexNotFound, got %v", err)
	}
}
