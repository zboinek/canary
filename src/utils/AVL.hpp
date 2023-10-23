/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */
#pragma once

template <typename T>
class MemoryPool {
private:
	struct Block {
		T data;
		Block* next = nullptr;
	};

	Block* freeList = nullptr;

public:
	~MemoryPool() {
		while (freeList) {
			Block* temp = freeList;
			freeList = freeList->next;
			delete temp;
		}
	}

	T* allocate() {
		if (freeList) {
			Block* block = freeList;
			freeList = freeList->next;
			return &block->data;
		} else {
			return &((new Block())->data);
		}
	}

	void deallocate(T* ptr) {
		auto* block = reinterpret_cast<Block*>(ptr);
		block->next = freeList;
		freeList = block;
	}
};

template <typename T, typename Comparator = std::less<>>
class AVLTree {
private:
	struct Node {
		T value;
		int height = 1;
		std::unique_ptr<Node> left = nullptr;
		std::unique_ptr<Node> right = nullptr;
	};

	MemoryPool<Node> nodePool;

	std::unique_ptr<Node> getNodeFromPool(const T &value) {
		Node* newNode = nodePool.allocate();
		newNode->value = value;
		newNode->height = 1;
		newNode->left = nullptr;
		newNode->right = nullptr;
		return std::unique_ptr<Node>(newNode);
	}

	void returnNodeToPool(std::unique_ptr<Node> &node) {
		nodePool.deallocate(node.release());
	}

	std::unique_ptr<Node> root;
	Comparator compare;

	std::unique_ptr<Node> rotateLeft(std::unique_ptr<Node> &root) {
		auto newRoot = std::move(root->right);
		root->right = std::move(newRoot->left);
		newRoot->left = std::move(root);

		int heightLeft = newRoot->left->left ? newRoot->left->left->height : 0;
		int heightRight = newRoot->left->right ? newRoot->left->right->height : 0;
		newRoot->left->height = 1 + std::max(heightLeft, heightRight);

		heightLeft = newRoot->left ? newRoot->left->height : 0;
		heightRight = newRoot->right ? newRoot->right->height : 0;
		newRoot->height = 1 + std::max(heightLeft, heightRight);

		return newRoot;
	}

	std::unique_ptr<Node> rotateRight(std::unique_ptr<Node> &root) {
		auto newRoot = std::move(root->left);
		root->left = std::move(newRoot->right);
		newRoot->right = std::move(root);

		int heightLeft = newRoot->right->left ? newRoot->right->left->height : 0;
		int heightRight = newRoot->right->right ? newRoot->right->right->height : 0;
		newRoot->right->height = 1 + std::max(heightLeft, heightRight);

		heightLeft = newRoot->left ? newRoot->left->height : 0;
		heightRight = newRoot->right ? newRoot->right->height : 0;
		newRoot->height = 1 + std::max(heightLeft, heightRight);

		return newRoot;
	}

	std::unique_ptr<Node> balance(std::unique_ptr<Node> &root) {
		int heightLeft = root->left ? root->left->height : 0;
		int heightRight = root->right ? root->right->height : 0;

		int balanceFactor = heightLeft - heightRight;

		root->height = 1 + std::max(heightLeft, heightRight);

		if (balanceFactor > 1) {
			if (root->left->left && root->left->right) {
				if (root->left->left->height >= root->left->right->height) {
					return rotateRight(root);
				}
			} else {
				return rotateRight(root);
			}
			root->left = rotateLeft(root->left);
			return rotateRight(root);
		}

		if (balanceFactor < -1) {
			if (root->right->right && root->right->left) {
				if (root->right->right->height >= root->right->left->height) {
					return rotateLeft(root);
				}
			} else {
				return rotateLeft(root);
			}
			root->right = rotateRight(root->right);
			return rotateLeft(root);
		}

		return std::move(root);
	}

	std::unique_ptr<Node> batchInsertHelper(const std::vector<T> &elements, int start, int end) {
		if (start > end) {
			return nullptr;
		}

		int mid = (start + end) / 2;

		auto node = getNodeFromPool(elements[mid]);

		node->left = batchInsertHelper(elements, start, mid - 1);
		node->right = batchInsertHelper(elements, mid + 1, end);

		int heightLeft = node->left ? node->left->height : 0;
		int heightRight = node->right ? node->right->height : 0;
		node->height = 1 + std::max(heightLeft, heightRight);

		int balanceFactor = heightLeft - heightRight;

		if (std::abs(balanceFactor) > 1) {
			return balance(node);
		}

		return node;
	}

	std::unique_ptr<Node> insert(std::unique_ptr<Node> &root, const T &value, bool allowDuplicates) {
		if (!root) {
			return getNodeFromPool(value);
		}

		int heightLeft = root->left ? root->left->height : 0;
		int heightRight = root->right ? root->right->height : 0;

		if (compare(value, root->value) || (allowDuplicates && !compare(root->value, value))) {
			root->left = insert(root->left, value, allowDuplicates);
			heightLeft = root->left->height;
		} else if (compare(root->value, value)) {
			root->right = insert(root->right, value, allowDuplicates);
			heightRight = root->right->height;
		}

		root->height = 1 + std::max(heightLeft, heightRight);

		int balanceFactor = heightLeft - heightRight;
		if (std::abs(balanceFactor) > 1) {
			return balance(root);
		}

		return std::move(root);
	}

	std::unique_ptr<Node> removeNode(std::unique_ptr<Node> &root, const T &value) {
		if (!root) {
			return nullptr;
		}

		if (compare(value, root->value)) {
			root->left = removeNode(root->left, value);
		} else if (compare(root->value, value)) {
			root->right = removeNode(root->right, value);
		} else {
			if (!root->left) {
				auto temp = std::move(root->right);
				returnNodeToPool(root);
				return temp;
			} else if (!root->right) {
				auto temp = std::move(root->left);
				returnNodeToPool(root);
				return temp;
			}

			Node* maxNode = root->left.get();
			while (maxNode->right) {
				maxNode = maxNode->right.get();
			}
			root->value = maxNode->value;
			root->left = removeNode(root->left, maxNode->value);
		}

		int heightLeft = root->left ? root->left->height : 0;
		int heightRight = root->right ? root->right->height : 0;

		root->height = 1 + std::max(heightLeft, heightRight);

		int balanceFactor = heightLeft - heightRight;
		if (std::abs(balanceFactor) > 1) {
			return balance(root);
		}

		return std::move(root);
	}

	inline bool search(const std::unique_ptr<Node> &root, const T &value) const {
		if (!root) {
			return false;
		}

		if (compare(value, root->value)) {
			return search(root->left, value);
		} else if (compare(root->value, value)) {
			return search(root->right, value);
		} else {
			return true;
		}
	}

	template <typename U>
	std::shared_ptr<U> searchObject(const std::unique_ptr<Node> &root, const T &value) const {
		if (!root) {
			return nullptr;
		}

		if (compare(value, root->value)) {
			return searchObject<U>(root->left, value);
		} else if (compare(root->value, value)) {
			return searchObject<U>(root->right, value);
		} else {
			return std::dynamic_pointer_cast<U>(root->value);
		}
	}

	void clearAndReturnToPool(std::unique_ptr<Node> &node) {
		if (!node) {
			return;
		}

		clearAndReturnToPool(node->left);
		clearAndReturnToPool(node->right);

		returnNodeToPool(node);
	}

public:
	AVLTree(Comparator comp = Comparator()) :
		compare(comp) { }
	~AVLTree() {
		clearAndReturnToPool(root);
	}

	void clearAll() {
		root.reset();
	}

	void preallocate(size_t N) {
		for (size_t i = 0; i < N; ++i) {
			auto tempNode = getNodeFromPool(T {});
			returnNodeToPool(tempNode);
		}
	}

	void batchInsert(const std::vector<T> &elements) {
		auto sorted_elements = elements;
		std::sort(sorted_elements.begin(), sorted_elements.end(), compare);

		root = batchInsertHelper(sorted_elements, 0, sorted_elements.size() - 1);
	}

	void insert(const T &value, bool allowDuplicates = false) {
		root = insert(root, value, allowDuplicates);
	}

	void remove(const T &value) {
		root = removeNode(root, value);
	}

	bool search(const T &value) const {
		return search(root, value);
	}

	template <typename U>
	std::shared_ptr<U> searchObject(const T &value) const {
		return searchObject<U>(root, value);
	}

	static void benchmarkInsert(AVLTree<T> &tree, int NUM_OPERATIONS) {
		std::default_random_engine generator;
		std::uniform_int_distribution<T> distribution(1, 1000000);

		Benchmark bm;
		for (int i = 0; i < NUM_OPERATIONS; ++i) {
			T value = distribution(generator);
			tree.insert(value);
		}
		g_logger().info("Tempo para inserção AVL: - {}ms", bm.duration());
	}

	static void benchmarkInsert1(AVLTree<T> &tree) {
		std::default_random_engine generator;
		std::uniform_int_distribution<T> distribution(1, 1000000);

		Benchmark bm;
		T value = distribution(generator);
		tree.insert(value);
		g_logger().info("Tempo para 1 inserção AVL: - {}ms", bm.duration());
	}

	static void benchmarkSearch(AVLTree<T> &tree, int NUM_OPERATIONS) {
		std::default_random_engine generator;
		std::uniform_int_distribution<T> distribution(1, 1000000);

		Benchmark bm;
		for (int i = 0; i < NUM_OPERATIONS; ++i) {
			T value = distribution(generator);
			tree.search(value);
		}
		g_logger().info("Tempo para busca AVL: - {}ms", bm.duration());
	}

	static void benchmarkSearch1(AVLTree<T> &tree) {
		std::default_random_engine generator;
		std::uniform_int_distribution<T> distribution(1, 1000000);

		Benchmark bm;
		T value = distribution(generator);
		tree.search(value);
		g_logger().info("Tempo para 1 busca AVL: - {}ms", bm.duration());
	}

	static void benchmarkRemove(AVLTree<T> &tree, int NUM_OPERATIONS) {
		std::default_random_engine generator;
		std::uniform_int_distribution<T> distribution(1, 1000000);

		Benchmark bm;
		for (int i = 0; i < NUM_OPERATIONS; ++i) {
			T value = distribution(generator);
			tree.remove(value);
		}
		g_logger().info("Tempo para remoção AVL: - {}ms", bm.duration());
	}

	static void benchmarkClearAll(AVLTree<T> &tree) {
		Benchmark bm;
		tree.clearAll();
		g_logger().info("Tempo para clearAll AVL: - {}ms", bm.duration());
	}

	static void benchmarkBatchInsert(AVLTree<T> &tree, int NUM_OPERATIONS) {
		std::default_random_engine generator;
		std::uniform_int_distribution<T> distribution(1, 1000000);

		// Gera um vetor de elementos aleatórios
		std::vector<T> elements(NUM_OPERATIONS);
		for (int i = 0; i < NUM_OPERATIONS; ++i) {
			elements[i] = distribution(generator);
		}

		Benchmark bm;
		tree.batchInsert(elements);
		g_logger().info("Tempo para inserção em lote AVL: - {}ms", bm.duration());
	}

	static void benchmarkAVLTree(AVLTree<T> &tree) {
		const int NUM_OPERATIONS = 1000000;
		tree.preallocate(NUM_OPERATIONS); // Prealocação de memória

		benchmarkInsert(tree, NUM_OPERATIONS); // Inserção individual
		benchmarkSearch(tree, NUM_OPERATIONS); // Busca
		benchmarkRemove(tree, NUM_OPERATIONS); // Remoção
		benchmarkInsert1(tree); // Inserção individual (uma operação)
		benchmarkSearch1(tree); // Busca (uma operação)
		benchmarkClearAll(tree); // Limpeza

		g_logger().info("Adiciono tudo novamente para medir o ClearAll");
		benchmarkInsert(tree, NUM_OPERATIONS); // Adiciono tudo novamente para medir o clearALL
		benchmarkClearAll(tree); // Limpeza após inserção

		g_logger().info("Benchmark para inserção em lote");
		benchmarkBatchInsert(tree, NUM_OPERATIONS); // Benchmark para inserção em lote
	}
};
