#include "tree.h"
#include <algorithm>
#include <functional>

// Конструктор
Tree::Tree() : root(nullptr), context(1), socket(context, zmq::socket_type::pub) { // zmq::socket_type::pub
    try {
        socket.bind("tcp://127.0.0.1:5555");  // Пример порта, при необходимости изменить
        std::cout << "Tree bound to tcp://127.0.0.1:5555" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Error binding Tree socket: " << e.what() << std::endl;
    }
}

// Деструктор для очистки дерева
Tree::~Tree() {
    std::lock_guard<std::mutex> lock(treeMutex);
    deleteTree(root);
    // Закрыть сокет и контекст
    socket.close();
    context.close();
}

// Рекурсивная функция для удаления дерева
void Tree::deleteTree(TreeNode* node) {
    if (node != nullptr) {
        deleteTree(node->left);
        deleteTree(node->right);
        node->node->stop(); // Остановить прослушивание узла
        delete node;
    }
}

// Получение высоты узла
int Tree::getHeight(TreeNode* N) {
    if (N == nullptr)
        return 0;
    return N->height;
}

// Получение баланса узла
int Tree::getBalance(TreeNode* N) {
    if (N == nullptr)
        return 0;
    return getHeight(N->left) - getHeight(N->right);
}

// Правый поворот
TreeNode* Tree::rightRotate(TreeNode* y) {
    TreeNode* x = y->left;
    TreeNode* T2 = x->right;

    // Выполнение поворота
    x->right = y;
    y->left = T2;

    // Обновление высот
    y->height = std::max(getHeight(y->left), getHeight(y->right)) + 1;
    x->height = std::max(getHeight(x->left), getHeight(x->right)) + 1;

    // Возврат нового корня
    return x;
}

// Левый поворот
TreeNode* Tree::leftRotate(TreeNode* x) {
    TreeNode* y = x->right;
    TreeNode* T2 = y->left;

    // Выполнение поворота
    y->left = x;
    x->right = T2;

    // Обновление высот
    x->height = std::max(getHeight(x->left), getHeight(x->right)) + 1;
    y->height = std::max(getHeight(y->left), getHeight(y->right)) + 1;

    // Возврат нового корня
    return y;
}

// Вставка узла в AVL-дерево
TreeNode* Tree::insert(TreeNode* node, int id, std::shared_ptr<Node> newNode, bool& success, std::string& errorMsg) {
    // Выполнение обычной вставки BST
    if (node == nullptr) {
        success = true;
        TreeNode* newTreeNode = new TreeNode(id, newNode);
        newTreeNode->parent = nullptr; // Устанавливаем parent, если требуется
        return newTreeNode;
    }

    if (id < node->id) {
        node->left = insert(node->left, id, newNode, success, errorMsg);
        if (node->left != nullptr)
            node->left->parent = node; // Устанавливаем parent
    }
    else if (id > node->id) {
        node->right = insert(node->right, id, newNode, success, errorMsg);
        if (node->right != nullptr)
            node->right->parent = node; // Устанавливаем parent
    }
    else { // Дублирующиеся id не допускаются
        success = false;
        errorMsg = "Error: Already exists";
        return node;
    }

    // Обновление высоты узла
    node->height = 1 + std::max(getHeight(node->left), getHeight(node->right));

    // Получение баланса узла
    int balance = getBalance(node);

    // Проверка баланса и выполнение поворотов при необходимости

    // Left Left Case
    if (balance > 1 && id < node->left->id)
        return rightRotate(node);

    // Right Right Case
    if (balance < -1 && id > node->right->id)
        return leftRotate(node);

    // Left Right Case
    if (balance > 1 && id > node->left->id) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    // Right Left Case
    if (balance < -1 && id < node->right->id) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    // Возврат неизменённого узла
    return node;
}

// Создание узла
bool Tree::createNode(int id, int parentId) {
    std::lock_guard<std::mutex> lock(treeMutex);

    static int basePort = 6000;  // Начальный порт

    // Проверка, существует ли узел с таким id
    if (findNode(id) != nullptr) {
        std::cerr << "Error: Already exists" << std::endl;
        return false;
    }

    // Если указан parentId, проверить существование и доступность родителя
    if (parentId != -1) {
        std::shared_ptr<Node> parentNode = findNode(parentId);
        if (parentNode == nullptr) {
            std::cerr << "Error: Parent not found" << std::endl;
            return false;
        }
        if (!parentNode->isAvailable) {
            std::cerr << "Error: Parent is unavailable" << std::endl;
            return false;
        }
    }

    // Создание нового узла
    std::shared_ptr<Node> newNode;
    try {
        newNode = std::make_shared<Node>(id, basePort, context);
    } catch (...) {
        std::cerr << "Error: Failed to create node " << id << std::endl;
        return false;
    }
    basePort += 2;  // Инкрементирование для следующего узла

    // Вставка узла в AVL-дерево
    bool success = false;
    std::string errorMsg;
    root = insert(root, id, newNode, success, errorMsg);
    if (!success) {
        std::cerr << errorMsg << std::endl;
        return false;
    }

    // Если указан parentId, добавить новый узел как дочерний
    if (parentId != -1) {
        auto parentTreeNode = findTreeNode(parentId);
        if (parentTreeNode) {
            parentTreeNode->node->addChild(newNode);
        }
    }

    // Запуск прослушивания узла в отдельном потоке
    std::thread(&Node::startListening, newNode).detach();

    // Получение PID текущего процесса
    pid_t pid = getpid();
    std::cout << "Ok:" << pid << std::endl;
    return true;
}

// Поиск узла по id
std::shared_ptr<Node> Tree::findNode(int id) {
    TreeNode* current = root;
    while (current != nullptr) {
        if (id < current->id) {
            current = current->left;
        }
        else if (id > current->id) {
            current = current->right;
        }
        else {
            return current->node;
        }
    }
    return nullptr;
}

// Поиск TreeNode по id
TreeNode* Tree::findTreeNode(int id) {
    TreeNode* current = root;
    while (current != nullptr) {
        if (id < current->id) {
            current = current->left;
        }
        else if (id > current->id) {
            current = current->right;
        }
        else {
            return current;
        }
    }
    return nullptr;
}

// Рекурсивная функция для отметки поддерева как недоступного
void Tree::markSubtreeUnavailable(TreeNode* node) {
    if (node == nullptr) return;
    {
        std::lock_guard<std::mutex> lock(treeMutex);
        node->node->isAvailable = false;
    }
    markSubtreeUnavailable(node->left);
    markSubtreeUnavailable(node->right);
}

// Обработка команды heartbeat
void Tree::handleHeartbeatCommand(int timeMs) {
    std::lock_guard<std::mutex> lock(treeMutex);
    // Запуск мониторинга heartbeat для каждого узла в отдельных потоках
    std::function<void(TreeNode*)> traverse = [&](TreeNode* node) {
        if (node == nullptr) return;
        std::thread(&Tree::monitorHeartbeat, this, node->id, timeMs).detach();
        traverse(node->left);
        traverse(node->right);
    };

    traverse(root);
    std::cout << "Ok" << std::endl;
}

// Мониторинг heartbeat-сигнала для узла
void Tree::monitorHeartbeat(int id, int interval) {
    auto node = findNode(id);
    if (!node) return;

    while (node->isAvailable) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));

        // Отправка heartbeat-сообщения
        try {
            zmq::message_t message("heartbeat", 9);
            socket.send(message, zmq::send_flags::none);
        }
        catch (const zmq::error_t& e) {
            std::cerr << "ZeroMQ Error while sending heartbeat for node " << id << ": " << e.what() << std::endl;
            continue;
        }

        // Проверка доступности узла
        if (!node->checkAvailability(interval)) {
            if (node->isAvailable) { // Отметить как недоступный только один раз
                node->isAvailable = false;
                std::cerr << "Heartbeat: node " << id << " is unavailable now" << std::endl;
                // Поиск TreeNode и отметка поддерева как недоступного
                TreeNode* treeNode = findTreeNode(id);
                if (treeNode) {
                    markSubtreeUnavailable(treeNode->left);
                    markSubtreeUnavailable(treeNode->right);
                }
            }
            break; // Прекратить мониторинг данного узла
        }
    }
}

// Обработка команды ping
bool Tree::handlePingCommand(int id) {
    auto node = findNode(id);
    if (node && node->isAvailable) {
        std::cout << "Ok:" << getpid() << std::endl; // Возвращаем PID текущего процесса
        return true;
    }
    std::cout << "Ok: 0" << std::endl;
    return false;
}

// Удаление узла по id (не реализовано)
bool Tree::removeNode(int id) {
    // Реализация удаления узла из AVL-дерева при необходимости
    // Для простоты данный функционал не реализован
    std::cerr << "Error: Remove node not implemented" << std::endl;
    return false;
}
