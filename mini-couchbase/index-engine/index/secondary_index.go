package index

import "sync"

type SecondaryIndex struct {
	mu      sync.RWMutex
	entries map[string]map[string]struct{}
}

func NewSecondaryIndex() *SecondaryIndex {
	return &SecondaryIndex{
		entries: make(map[string]map[string]struct{}),
	}
}

// Insert associates an indexed value with a document key.
func (idx *SecondaryIndex) Insert(value, documentKey string) {
	idx.mu.Lock()
	defer idx.mu.Unlock()

	if idx.entries[value] == nil {
		idx.entries[value] = make(map[string]struct{})
	}

	idx.entries[value][documentKey] = struct{}{}
}

// Delete removes the association between a value and a document key.
func (idx *SecondaryIndex) Delete(value, documentKey string) {
	idx.mu.Lock()
	defer idx.mu.Unlock()

	documents := idx.entries[value]
	if documents == nil {
		return
	}

	delete(documents, documentKey)

	if len(documents) == 0 {
		delete(idx.entries, value)
	}
}

// Lookup returns all document keys associated with an indexed value.
func (idx *SecondaryIndex) Lookup(value string) []string {
	idx.mu.RLock()
	defer idx.mu.RUnlock()

	documents := idx.entries[value]
	result := make([]string, 0, len(documents))

	for documentKey := range documents {
		result = append(result, documentKey)
	}

	return result
}
