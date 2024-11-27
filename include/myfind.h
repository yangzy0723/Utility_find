#ifndef DEFINE_H
#define DEFINE_H

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @struct node
 * @brief 描述文件系统中的一个节点信息。
 *
 * 该结构体表示文件系统中的一个节点（如文件、目录或符号链接）及其相关属性。
 */
struct node
{
    char *name;       /**< 节点的完整路径名，包括路径和文件名。 */
    char *name_wp;    /**< 节点的文件名，不含路径部分（name without path）。 */
    mode_t type;      /**< 节点的链接类型，来自 `lstat` 系统调用的结果。 */
    mode_t r_type;    /**< 节点的实际类型，来自 `stat` 系统调用的结果。 */
};

/**
 * @enum enum_type
 * @brief 表示特定操作或条件类型的枚举。
 *
 * 该枚举定义了一组操作或条件，用于解析或执行逻辑操作。
 */
enum enum_type
{
    THEN = 0,       /**< 表示 "THEN" 操作，逻辑上的 "然后"。 */
    AND,            /**< 表示 "AND" 操作，逻辑上的 "与"。 */
    OR,             /**< 表示 "OR" 操作，逻辑上的 "或"。 */
    NO,             /**< 表示 "NO" 操作，逻辑上的 "非"（否定）。 */
    PRINT,          /**< 表示打印操作。 */
    CONDITION,      /**< 表示一个条件操作。 */
    EXEC,           /**< 表示执行操作（普通执行）。 */
    EXECP,          /**< 表示执行操作（带有额外参数）。 */
    PAO,            /**< 表示左括号 "("（parenthesis open）。 */
    PAC,            /**< 表示右括号 ")"（parenthesis close）。 */
    FAPA            /**< 表示因式分解括号（factorized parenthesis）。 */
};

/**
 * @struct compound
 * @brief 表示一个复合命令或逻辑单元的数据结构。
 *
 * 该结构体用于存储命令、其参数，以及与特定操作类型相关的逻辑结构。
 */
struct compound
{
    char *name;             /**< 命令的名称，例如 "-name" 或 "-exec"。 */
    char **args;            /**< 命令的参数数组，以 NULL 结尾。 */
    struct compound **fapa; /**< 指向复合命令数组的指针，适用于 `et == FAPA` 的情况。 */
    enum enum_type et;      /**< 枚举值，表示该复合命令的逻辑类型（如 OR、AND 等）。 */
};

/**
 * @struct ast
 * @brief 表示抽象语法树（AST）节点的数据结构。
 *
 * 该结构体用于存储表达式或命令的抽象语法树节点，其中每个节点可以有左右子节点，表示树的层次结构。
 * 该结构支持表达式树或命令链的解析和执行。
 */
struct ast
{
    struct ast *left;         /**< 指向左子节点的指针，表示树的左侧操作或表达式。 */
    struct ast *right;        /**< 指向右子节点的指针，表示树的右侧操作或表达式。 */
    enum enum_type et;        /**< 当前节点的操作类型，表示该节点的操作（如 AND、OR、THEN 等）。 */
    int rvalue[2];            /**< `rvalue` 数组，存储左右子节点的返回值：
                                 * - `rvalue[0]`: 左子节点的返回值。
                                 * - `rvalue[1]`: 右子节点的返回值。
                                 */
    struct compound **c_list; /**< 指向复合命令列表的指针数组。 */
    size_t cl_size;           /**< `c_list` 的大小，表示命令列表的元素数量。 */
};

/**
 * @struct data
 * @brief 存储程序运行状态、命令执行信息及相关数据结构。
 *
 * 该结构体用于管理命令行选项、命令列表、表达式、AST 树、以及避免循环遍历的 inode 信息等。
 */
struct data
{
    int return_value;       /**< 存储程序的返回值（例如命令执行后的退出状态）。 */
    int d_checked;          /**< 标记是否开启-d选项。 */
    int option;             /**< 存储命令行选项：
                              * - 0: -P 默认行为，不跟随符号链接，仅处理符号链接本身
                              * - 1: -H 命令行中明确指定的符号链接会被跟踪到它们指向的文件或目录
                              * - 2: -L 无论是命令行中指定的符号链接，还是在遍历目录时遇到的符号链接，都会被跟踪 */
    
    // 查找路径列表
    char **name_list;       /**< 存储命令行中输入的查找路径，通常用于目录、文件等的路径。 */
    size_t nl_size;         /**< `name_list` 中元素的当前数量。 */
    size_t nl_capacity;     /**< `name_list` 当前分配的容量，表示最多能容纳多少元素。 */
    
    // 表达式列表
    char **e_list;          /**< 存储表达式的列表，例如逻辑表达式等。 */
    size_t el_size;         /**< `e_list` 中元素的当前数量。 */
    size_t el_capacity;     /**< `e_list` 当前分配的容量，表示最多能容纳多少表达式。 */
    
    // 节点列表
    struct node **nodes;    /**< 存储节点列表，表示各个文件、目录或符号链接。 */
    size_t no_size;         /**< `nodes` 中元素的当前数量。 */
    size_t no_capacity;     /**< `nodes` 当前分配的容量，表示最多能容纳多少节点。 */
    
    // 存储已访问目录的 inode 列表，避免无限循环
    int *inode_list;        /**< 存储已访问目录的 inode 列表，用于避免无限循环遍历。 */
    size_t il_size;         /**< `inode_list` 中元素的当前数量。 */
    size_t il_capacity;     /**< `inode_list` 当前分配的容量，表示最多能容纳多少 inode。 */
    
    // 存储复合命令列表（如因式分解的表达式）
    struct compound **c_list; /**< 存储因式分解的表达式列表，在构建 AST 前使用。 */
    size_t cl_size;         /**< `c_list` 中元素的当前数量。 */
    size_t cl_capacity;     /**< `c_list` 当前分配的容量，表示最多能容纳多少复合命令。 */
    
    // 抽象语法树（AST）
    struct ast *ast;        /**< 存储抽象语法树，用于表示命令或表达式的树状结构。 */
    
    // 动作标记
    int actions;            /**< 如果 AST 中包含动作（如打印、执行），则为 1；否则为 0。 */
};

/**
 * @brief 初始化 `struct data` 结构。
 *
 * 该函数为 `struct data` 分配所需的动态内存，并初始化相关字段为默认值。
 * 所有动态数组的初始容量为 10，`ast` 初始化为空树。
 *
 * @param d 指向需要初始化的 `struct data` 的指针。
 * @note 使用此函数后，调用者应负责在使用完成后释放 `struct data` 的资源。
 */
void init_data(struct data *d);

/**
 * @brief 检查并更新选项标志
 *
 * 此函数根据传入的选项字符串 `opt`，检查其是否匹配某个预定义的选项标志。
 * 如果匹配，则根据该选项更新 `struct data` 结构体中的相关字段。
 *
 * - 如果选项为 `-d`，将 `d->d_checked` 设置为 `1`。
 * - 如果选项为 `-P`、`-H` 或 `-L`，将 `d->option` 设置为不同的值。
 * - -H、-L 和 -P 同时指定，最后一个指定的选项生效。
 * @param d 要更新的 `struct data` 结构体。
 * @param opt 传入的选项字符串。
 *
 * @return 如果选项匹配并成功更新字段，返回 `1`；否则返回 `0`，表示没有匹配的选项。
 */
int update_option(struct data *d, char *opt);

/**
 * @brief 生成节点并递归解析目录
 *
 * 该函数遍历 `name_list` 中的所有文件或目录，并根据文件信息生成节点。如果文件是符号链接，
 * 会获取符号链接和目标文件的状态信息。如果文件是目录并且符号链接没有循环或符合特定选项，
 * 则递归调用 `parse_dir` 来遍历子目录。对于每个文件或目录，都会调用 `add_node` 来将其信息
 * 添加到数据结构中。函数还会确保不重复解析相同的 inode。
 *
 * @param d 指向 `struct data` 的指针，包含 `name_list` 数组和相关容量信息。
 */
void generate_nodes(struct data *d);

/**
 * @brief 遍历表达式列表（e_list），并根据每个表达式的类型创建相应的复合表达式，将其添加到复合表达式列表（c_list）中。
 *
 * 该函数会遍历 `data` 结构体中的表达式列表（`e_list`），
 * 根据每个表达式的类型（例如 `-print`, `-a`, `-type`, `-exec` 等），创建相应的复合表达式，
 * 并将其添加到 `c_list` 中。支持的表达式包括打印、逻辑操作符、括号、条件表达式以及执行。
 * 如果某些表达式语法不正确，函数会打印错误信息并返回错误代码 1。否则，返回 0。
 *
 * @param d 指向 `data` 结构体的指针，包含表达式列表（e_list）和复合表达式列表（c_list）。
 *
 * @return 如果成功，返回 0；如果出现错误（例如无效的语法），返回 1。
 */
int create_c_list(struct data *d);

/**
 * @brief 构建抽象语法树 (AST)。
 *
 * 该函数用于构建一个抽象语法树（AST），它根据命令的逻辑结构（如逻辑操作符、括号等）递归地构建子树。
 * 首先，它会根据输入的命令（存储在 `ast->c_list` 中）处理逻辑操作符（如 `OR`、`AND`、`THEN`）并分割成左右子树。
 * 其次，函数处理括号和条件结构，构建相应的子树以表示命令之间的依赖关系。
 *
 * @param ast 指向抽象语法树 (AST) 结构体的指针。该结构体包含了命令列表（`c_list`）和与命令相关的 AST 节点信息。
 *
 * @return 无返回值。该函数直接修改传入的 `ast` 结构体，构建 AST。
 */
void build_ast(struct ast *ast);

/**
 * @brief 执行抽象语法树（AST）中的操作。
 *
 * 该函数根据 AST 节点的类型（如 THEN、AND、OR、PRINT、CONDITION、EXECP 和 EXEC 执行不同的操作，
 * 处理节点的条件判断、命令执行等。
 *
 * @param parent 父节点指针，用于更新父节点的返回值。
 * @param ast 当前执行的节点（AST）指针。
 * @param n 当前处理的节点信息（如文件名、类型等）。
 * @param child 当前节点是父节点的左子节点（child = 0）还是右子节点（child = 1）。
 *
 * @return 返回执行结果，表示是否成功执行某个操作。
 */
int exec_ast(struct ast *parent, struct ast *ast, struct node *n, int child);

/**
 * @brief 动态扩展 `struct data` 中的数组容量
 *
 * 根据 `id` 参数的不同，该函数会重新分配 `struct data` 中不同数组的内存。
 * 每次重新分配时，都会将容量翻倍，并使用 `realloc` 扩展相应的数组。
 *
 * @param d 指向 `struct data` 的指针，包含需要扩展的数组。
 * @param id 标识需要扩展的数组。不同的 `id` 对应不同的数组：
 *          - id = 0 扩展 `name_list` 数组
 *          - id = 1 扩展 `e_list` 数组
 *          - id = 2 扩展 `nodes` 数组
 *          - id = 3 扩展 `inode_list` 数组
 *          - id = 4 扩展 `c_list` 数组
 */
void my_realloc(struct data *d, int id);

/**
 * @brief 将搜索路径名称添加到 `d->name_list` 数组
 *
 * 该函数用于将给定的名称 `name` 添加到 `struct data` 中的 `name_list` 数组。
 * 如果当前数组容量已满，函数将调用 `my_realloc` 扩展数组容量，并继续将名称添加到数组。
 *
 * @param d 指向 `struct data` 的指针，包含 `name_list` 数组和相关的容量信息（`nl_size` 和 `nl_capacity`）。
 * @param name 要添加到 `name_list` 数组中的名称字符串。此名称将被复制并存储在数组中。
 */
void add_search_path(struct data *d, char *name);

/**
 * @brief 将表达式添加到 `d->e_list` 数组
 *
 * 该函数用于将传入的表达式 `exp` 添加到 `struct data` 中的 `e_list` 数组。
 * 如果当前数组的容量已满，函数会调用 `my_realloc` 来扩展数组容量，并继续将表达式添加到数组中。
 *
 * @param d 指向 `struct data` 的指针，其中包含 `e_list` 数组和相关容量信息（`el_size` 和 `el_capacity`）。
 * @param exp 要添加到 `e_list` 数组的表达式字符串。
 */
void add_exp(struct data *d, char *exp);

/**
 * @brief 将 inode 添加到 `inode_list` 数组，以避免目录遍历中的无限循环
 *
 * 该函数用于将目录的 inode 值 `ino` 添加到 `struct data` 中的 `inode_list` 数组。
 * `inode_list` 用于记录已经遍历过的目录的 inode，从而避免在遍历过程中进入相同的目录，防止无限循环。
 * 如果当前数组容量已满，函数会调用 `my_realloc` 来扩展数组容量，并继续将 inode 添加到数组中。
 *
 * @param ino 目录的 inode 值。
 * @param d 指向 `struct data` 的指针，其中包含 `inode_list` 数组和相关容量信息（`il_size` 和 `il_capacity`）。
 */
void add_inode(int ino, struct data *d);

/**
 * @brief 检查指定的 inode 是否已经存在于 `inode_list` 中
 *
 * 该函数检查给定的 inode 编号 `ino` 是否已经存在于 `d->inode_list` 数组中。
 * 它通过遍历 `inode_list` 数组来查找是否有与 `ino` 相同的 inode 值。如果找到了，则返回 1，表示该 inode 已经存在；如果没有找到，则返回 0，表示该 inode 不存在。
 *
 * @param ino 要检查的 inode 编号。
 * @param d 指向 `struct data` 的指针，其中包含 `inode_list` 数组和相关的信息。
 * @return 如果 `ino` 存在于 `inode_list` 中，返回 1；否则返回 0。
 */
int inode_exists(int ino, struct data *d);

/**
 * @brief 将一个新的节点添加到 `nodes` 数组
 *
 * 该函数用于将一个新的 `node` 结构体添加到 `struct data` 的 `nodes` 数组中。`node` 结构体包含文件/目录的信息，如名称、类型等。
 * 如果当前 `nodes` 数组的容量已满，函数会调用 `my_realloc` 来扩展数组的容量，然后将新的节点添加进去。
 *
 * @param name 文件/目录的名称。
 * @param name_wp 文件/目录的名称，不包含路径。
 * @param type 文件/目录的类型（使用 `mode_t` 类型，包含文件的类型信息）。
 * @param r_type 文件/目录的原始类型（使用 `mode_t` 类型，表示文件的原始类型信息）。
 * @param d 指向 `struct data` 的指针，其中包含 `nodes` 数组和相关容量信息（`no_size` 和 `no_capacity`）。
 */
void add_node(char *name, char *name_wp, mode_t type, mode_t r_type, struct data *d);

/**
 * @brief 将一个新的复合命令（compound）添加到 data 结构体中的 c_list（复合命令列表）。
 *
 * 该函数创建一个新的复合命令，将其添加到 `data` 结构体中的 `c_list`（复合命令列表）。首先，函数会分配内存并初始化复合命令结构体，设置命令的名称、参数和命令类型。接着，如果 `c_list` 的当前容量不足以容纳新的命令，则会调用 `my_realloc` 函数重新分配内存。
 *
 * @param d 指向 `data` 结构体的指针，包含复合命令列表（c_list）。
 * @param name 新复合命令的名称。
 * @param args 新复合命令的参数数组。
 * @param et 新复合命令的类型（枚举类型 `enum_type`）。
 */
void add_compound(struct data *d, char *name, char **args, enum enum_type et);

/**
 * @brief 递归遍历指定目录并处理每个文件或子目录
 *
 * 该函数递归地遍历给定目录 `name`，对目录中的每个文件和子目录执行特定操作。在遍历过程中，
 * 会为每个文件或子目录执行一些操作，如将文件或目录的信息添加到数据结构中。对于符号链接，函数会
 * 检查符号链接本身以及符号链接指向的目标文件的类型。该函数会跳过当前目录 (`.`) 和父目录 (`..`)。
 *
 * @param name 要遍历的目录的路径，需要保证是有效目录。
 * @param d 指向 `struct data` 的指针，包含需要的数组和信息，用于存储遍历结果。
 */
void parse_dir(char *name, struct data *d);

/**
 * @brief 将一个新的复合命令（compound）添加到 data 结构体中的 c_list（复合命令列表）。
 *
 * 该函数创建一个新的复合命令，将其添加到 `data` 结构体中的 `c_list`（复合命令列表）。首先，函数会分配内存并初始化复合命令结构体，设置命令的名称、参数和命令类型。接着，如果 `c_list` 的当前容量不足以容纳新的命令，则会调用 `my_realloc` 函数重新分配内存。
 *
 * @param d 指向 `data` 结构体的指针，包含复合命令列表（c_list）。
 * @param name 新复合命令的名称。
 * @param args 新复合命令的参数数组。
 * @param et 新复合命令的类型（枚举类型 `enum_type`）。
 */
void add_compound(struct data *d, char *name, char **args, enum enum_type et);

/**
 * @brief 在抽象语法树 (AST) 中找到匹配的右括号 (PAC)。
 *
 * 该函数用于在 AST 中找到与给定的左括号 (PAO) 相对应的右括号 (PAC)。
 *
 * @param ast 指向抽象语法树 (AST) 结构体的指针。
 * @param i 起始位置的索引，从此位置开始查找匹配的右括号。
 *
 * @return 匹配的右括号的索引位置。
 */
int find_close(struct ast *ast, int i);

/**
 * @brief 在抽象语法树 (AST) 中查找逻辑操作符（如 AND 或 OR），并根据找到的操作符将 AST 拆分成左右子树。
 *
 * 该函数会遍历 AST 的复合命令列表（`c_list`），查找类型为 `type` 的逻辑操作符（如 AND 或 OR）。
 * 当找到该操作符时，函数会根据其位置将 AST 分成左右子树。
 * 左子树包含操作符左边的所有命令，右子树包含操作符右边的所有命令。
 * 函数会将拆分后的子树分配给 `ast->left` 和 `ast->right`。
 *
 * @param ast 指向抽象语法树 (AST) 结构体的指针。
 * @param type 要查找的操作符类型，通常为 `AND` 或 `OR`（由枚举 `type` 指定）。
 *
 * @return 如果找到匹配的操作符并成功拆分 AST，返回 1；否则返回 0。
 */
int search_OR_AND(struct ast *ast, unsigned int type);

/**
 * @brief 替换字符串中的占位符 `{}` 为给定的名称字符串。
 *
 * 该函数会扫描输入的字符串 `str`，寻找形式为 `{}` 的占位符，并将这些占位符替换为给定的字符串 `name`。
 * 在找到占位符后，它会将 `{}` 替换为 `name`，然后继续处理原始字符串的其余部分。
 * 最终返回一个新的字符串，包含所有替换后的结果。
 *
 * @param str 输入的字符串，其中可能包含 `{}` 占位符。
 * @param name 要替换占位符的名称字符串。
 *
 * @return 返回一个新的字符串，替换后的结果。该字符串由 `calloc` 分配内存，调用者需要在适当时机释放该内存。
 */
char *replace_echo(char *str, char *name);

/**
 * @brief 查找字符串中是否存在一对大括号 `{}`。
 *
 * 该函数用于检查给定的字符串 `str` 中是否包含一对紧密相连的大括号 `{}`。如果存在，返回 0；否则返回 1。
 *
 * @param str 输入的字符串，其中可能包含一对大括号 `{}`。
 *
 * @return 如果字符串中包含大括号 `{}`，返回 0；否则返回 1。
 */
int brackets_finder(char *str);

/**
 * @brief 验证抽象语法树（AST）的结构和逻辑。
 *
 * 该函数检查抽象语法树的有效性，通过检查每个节点的类型和结构，确保其符合预期的约束条件。
 * 该验证是递归进行的，若发现任何无效的节点，则返回非0，若树有效则返回0。
 *
 * - THEN节点如果存在，必须具有左子节点。
 * - AND和OR节点必须有左右两个子节点。
 * - NO节点必须处于有效的位置。
 *
 * 该函数还会在遇到PRINT或EXEC节点时，修改`d->actions`标志。
 *
 * @param d 指向数据结构的指针，该结构可能会被修改（例如设置actions标志）。
 * @param parent 当前节点的父节点。
 * @param ast 当前被验证的节点。
 * @param child 指示当前节点是父节点的左子节点（0）还是右子节点（1）的整数。
 *
 * @return 如果AST有效，返回0；如果无效，返回非0。
 */
int is_ast_valid(struct data *d, struct ast *parent, struct ast *ast, int child);

/**
 * @brief 重置抽象语法树（AST）中所有节点的rvalue值。
 *
 * 该函数通过深度优先遍历AST，重置每个节点的rvalue[0]和rvalue[1]为1。
 * rvalue字段表示节点的结果或状态，调用此函数后，将这些值恢复为初始状态。
 *
 * @param root 指向AST根节点的指针。
 */
void reset_rvalues(struct ast *root);

/**
 * @brief 释放并重新初始化 `inode_list` 数组
 *
 * 该函数用于释放 `struct data` 结构体中的 `inode_list` 数组所占用的内存，并重新初始化该数组。
 * 在释放内存后，`inode_list` 数组会被分配一个新的、容量为 10 的内存块，并将相关的计数器（`il_size` 和 `il_capacity`）重置为初始值。
 *
 * @param d 指向 `struct data` 的指针，其中包含 `inode_list` 数组以及相关的容量和大小信息。
 */
void free_il(struct data *d);

/**
 * @brief 递归释放抽象语法树 (AST) 的内存。
 *
 * 该函数通过递归方式释放抽象语法树 (AST) 中的节点及其相关联的资源。首先递归释放树的左子树和右子树，然后释放当前节点及其相关的资源。
 *
 * @param ast 要释放的 AST 结构体指针。
 */
void free_ast(struct ast *ast);

/**
 * @brief 释放数据结构中所有动态分配的内存。
 *
 * 该函数遍历并释放数据结构 `data` 中的各个成员，确保所有动态分配的内存都被正确释放。包括释放节点名称、节点结构体、节点列表、边列表、索引列表、名称列表以及命令列表等。
 *
 * @param d 要释放的 `data` 结构体指针。
 */
void free_data(struct data *d);

/**
 * @brief 打印抽象语法树（AST）的结构。
 *
 * 该函数递归地遍历并打印抽象语法树中每个节点的类型。对于每个节点，打印其在树中的索引、位置（左子节点或右子节点）以及节点的类型（如 THEN、AND、OR 等）。
 * 打印的输出有助于调试和可视化AST的结构。
 *
 * 该函数会递归调用自身来遍历左右子树，直到到达叶子节点。
 *
 * @param ast 指向当前节点的指针。
 * @param i 当前节点的索引（用于打印当前节点在树中的位置）。
 * @param side 当前节点是父节点的左子节点（side == 0）还是右子节点（side == 1）。
 */
void print_ast(struct ast *ast, int i, int side);
#endif