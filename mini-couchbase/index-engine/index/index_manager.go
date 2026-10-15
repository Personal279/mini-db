package index

import (
	"errors"
	"sync"
)

var ErrIndexExists = errors.New("index already exists")
var ErrIndexNotFound = errors.New("index not found")

type IndexManager struct {
	mu      sync.RWMutex
	indexes map[string]*SecondaryIndex
}

func NewIndexManager() *IndexManager {
	return &IndexManager{
		indexes: make(map[string]*SecondaryIndex),
	}
}

func (m *IndexManager) CreateIndex(name string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	if _, exists := m.indexes[name]; exists {
		return ErrIndexExists
	}

	m.indexes[name] = NewSecondaryIndex()
	return nil
}

func (m *IndexManager) GetIndex(name string) (*SecondaryIndex, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	idx, exists := m.indexes[name]
	if !exists {
		return nil, ErrIndexNotFound
	}

	return idx, nil
}

func (m *IndexManager) DropIndex(name string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	if _, exists := m.indexes[name]; !exists {
		return ErrIndexNotFound
	}

	delete(m.indexes, name)
	return nil
}
