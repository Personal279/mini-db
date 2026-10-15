package index

import (
	"reflect"
	"sort"
	"testing"
)

func assertKeys(t *testing.T, idx *SecondaryIndex, value string, expected []string) {
	t.Helper()

	got := idx.Lookup(value)

	sort.Strings(got)
	sort.Strings(expected)

	if !reflect.DeepEqual(got, expected) {
		t.Fatalf(
			"Lookup(%q) = %v; expected %v",
			value,
			got,
			expected,
		)
	}
}

func TestInsertAndLookup(t *testing.T) {
	idx := NewSecondaryIndex()

	idx.Insert("Bengaluru", "user:1")
	idx.Insert("Bengaluru", "user:2")
	idx.Insert("Udupi", "user:3")

	assertKeys(t, idx, "Bengaluru", []string{"user:1", "user:2"})
	assertKeys(t, idx, "Udupi", []string{"user:3"})
	assertKeys(t, idx, "Mysuru", []string{})
}

func TestDuplicateInsert(t *testing.T) {
	idx := NewSecondaryIndex()

	idx.Insert("Bengaluru", "user:1")
	idx.Insert("Bengaluru", "user:1")
	idx.Insert("Bengaluru", "user:1")

	assertKeys(t, idx, "Bengaluru", []string{"user:1"})
}

func TestDeleteAssociation(t *testing.T) {
	idx := NewSecondaryIndex()

	idx.Insert("Bengaluru", "user:1")
	idx.Insert("Bengaluru", "user:2")

	idx.Delete("Bengaluru", "user:1")

	assertKeys(t, idx, "Bengaluru", []string{"user:2"})
}

func TestDeleteLastAssociation(t *testing.T) {
	idx := NewSecondaryIndex()

	idx.Insert("Udupi", "user:1")
	idx.Delete("Udupi", "user:1")

	assertKeys(t, idx, "Udupi", []string{})
}

func TestDeleteNonexistentAssociation(t *testing.T) {
	idx := NewSecondaryIndex()

	// Deleting an unknown association should be harmless.
	idx.Delete("Bengaluru", "user:1")

	assertKeys(t, idx, "Bengaluru", []string{})

	idx.Insert("Bengaluru", "user:2")
	idx.Delete("UnknownCity", "user:2")

	assertKeys(t, idx, "Bengaluru", []string{"user:2"})
}

func TestSameDocumentMultipleValues(t *testing.T) {
	idx := NewSecondaryIndex()

	idx.Insert("Bengaluru", "user:1")
	idx.Insert("Udupi", "user:1")

	assertKeys(t, idx, "Bengaluru", []string{"user:1"})
	assertKeys(t, idx, "Udupi", []string{"user:1"})

	idx.Delete("Bengaluru", "user:1")

	assertKeys(t, idx, "Bengaluru", []string{})
	assertKeys(t, idx, "Udupi", []string{"user:1"})
}
