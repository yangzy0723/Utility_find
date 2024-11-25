#include "myfind.h"
#include "lib/lib_str.h"
#include "lib/lib_util.h"

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    // 初始化
    struct data d;
    init_data(&d);

    int index = 1;
    // 解析选项，如-d、-P、-H、-L
    for (; index < argc && argv[index][0] == '-'; index++)
        if (!update_option(&d, argv[index]))
            break;
    // 解析查找路径
    for (; index < argc && argv[index][0] != '-' && argv[index][0] != '(' && argv[index][0] != '!'; index++)
        add_search_path(&d, argv[index]);
    // 如果查找路径未指出，查找当前目录
    if (d.nl_size == 0)
    {
        d.nl_size = 1;
        d.name_list[0] = my_strcp(".");
    }
    // 解析表达式
    for (; index < argc; index++)
    {
        char *exp = my_strcp(argv[index]);
        add_exp(&d, exp);
    }

    generate_nodes(&d);
    // 打开选项-d，myfind会在处理目录的内容之前先处理该目录本身，因此invert nodes_list
    if (d.d_checked)
        invert_node_list(&d);

    // 创建命令列表并检查错误
    if (create_c_list(&d))
    {
        int rv = d.return_value;
        d.ast->c_list = d.c_list;
        free_data(&d);
        return rv;
    }

    // 全力处理d.ast，抽象语法树
    d.ast->c_list = d.c_list;
    d.ast->cl_size = d.cl_size;
    build_ast(d.ast);
    reset_rvalues(d.ast);

    if (is_ast_valid(&d, NULL, d.ast, 0))
    {
        free(d.c_list);
        free_data(&d);
        fprintf(stderr, "Expressions error\n");
        return 1;
    }
    for (int i = 0; i < d.no_size; i++)
    {
        if (d.ast->left)
            exec_ast(d.ast, d.ast, d.nodes[i], 0);
        if (!d.actions && d.ast->rvalue[0] == 1 && d.ast->rvalue[1] == 1)
            printf("%s\n", d.nodes[i]->name);
        reset_rvalues(d.ast);
    }

    int rvalue = d.return_value;
    free_data(&d);
    return (rvalue);
}

// Inits data structure
void init_data(struct data *d)
{
    d->option = 0;
    d->return_value = 0;
    d->d_checked = 0;
    d->nl_size = 0;
    d->el_size = 0;
    d->no_size = 0;
    d->il_size = 0;
    d->name_list = calloc(10, sizeof(char *));
    d->e_list = calloc(10, sizeof(char *));
    d->nodes = calloc(10, sizeof(struct node *));
    d->inode_list = calloc(10, sizeof(int));
    d->c_list = calloc(10, sizeof(struct compound *));
    d->nl_capacity = 10;
    d->el_capacity = 10;
    d->no_capacity = 10;
    d->il_capacity = 10;
    d->cl_size = 0;
    d->cl_capacity = 10;
    d->actions = 0;
    d->ast = calloc(1, sizeof(struct ast));
    d->ast->left = NULL;
    d->ast->right = NULL;
}

int update_option(struct data *d, char *opt)
{
    if (my_strcmp("-d", opt) == 0)
    {
        d->d_checked = 1;
        return 1;
    }
    else if (my_strcmp("-P", opt) == 0)
    {
        d->option = 0;
        return 1;
    }
    else if (my_strcmp("-H", opt) == 0)
    {
        d->option = 1;
        return 1;
    }
    else if (my_strcmp("-L", opt) == 0)
    {
        d->option = 2;
        return 1;
    }
    return 0;
}

void generate_nodes(struct data *d)
{
    struct stat sb;  // 用于保存文件信息
    struct stat sbl; // 用于保存符号链接信息
    mode_t types[2]; // 存储文件和符号链接的类型
    int srl;         // lstat return value
    int islnk;       // Used to avoid calling !S_ISLNK multiple time
    for (size_t i = 0; i < d->nl_size; i++)
    {
        srl = lstat(d->name_list[i], &sbl); // Info about link
        if (srl == -1)
        {
            fprintf(stderr, "\'%s\' : No such file or directory\n", d->name_list[i]);
            d->return_value = 1;
            continue;
        }
        stat(d->name_list[i], &sb); // Info about file behind link
        islnk = S_ISLNK(sbl.st_mode);
        types[0] = sbl.st_mode;
        types[1] = sb.st_mode;
        add_node(d->name_list[i], my_strcp(d->name_list[i]), types[0], types[1], d);
        if (!S_ISDIR(sb.st_mode))
            continue;
        // d->option == 1，-H：命令行中明确指定的符号链接会被跟踪到它们指向的文件或目录
        // d->option == 2，-L：无论是命令行中指定的符号链接，还是在遍历目录时遇到的符号链接，都会被跟踪
        else if (!islnk || d->option == 1 || d->option == 2)
        {
            add_inode(sb.st_ino, d);
            parse_dir(d->name_list[i], d);
            free_il(d); // free inode list for next usage
        }
    }
}

int create_c_list(struct data *d)
{
    char **args;
    for (size_t i = 0; i < d->el_size; i++)
    {
        if (my_strcmp("-print", d->e_list[i]) == 0)
            add_compound(d, d->e_list[i], NULL, PRINT);
        else if (my_strcmp("-a", d->e_list[i]) == 0)
            add_compound(d, d->e_list[i], NULL, AND);
        else if (my_strcmp("-o", d->e_list[i]) == 0)
            add_compound(d, d->e_list[i], NULL, OR);
        else if (my_strcmp("!", d->e_list[i]) == 0)
            add_compound(d, d->e_list[i], NULL, NO);
        else if (my_strcmp("(", d->e_list[i]) == 0)
            add_compound(d, d->e_list[i], NULL, PAO);
        else if (my_strcmp(")", d->e_list[i]) == 0)
            add_compound(d, d->e_list[i], NULL, PAC);
        else if (my_strcmp("-type", d->e_list[i]) == 0 ||
                 my_strcmp("-name", d->e_list[i]) == 0 ||
                 my_strcmp("-perm", d->e_list[i]) == 0)
        {
            if (i >= d->el_size - 1)
            {
                d->return_value = 1;
                fprintf(stderr, "Invalid condition syntaxe\n");
                return 1;
            }
            args = calloc(2, sizeof(char *));
            args[0] = d->e_list[i + 1];
            add_compound(d, d->e_list[i], args, CONDITION);
            i++;
        }
        else
        {
            size_t index = 1;
            while (my_strcmp(";", d->e_list[index + i]) && my_strcmp("+", d->e_list[index + i])) // Need handle wrong commands
            {
                index++;
                if (index >= d->el_size)
                {
                    d->return_value = 1;
                    fprintf(stderr, "-exec invalid syntaxe\n");
                    return 1;
                }
            }
            args = calloc(index, sizeof(char *));
            for (size_t j = 0; j < index - 1; j++)
                args[j] = d->e_list[i + j + 1];
            if (my_strcmp(";", d->e_list[index + i]))
                add_compound(d, d->e_list[i], args, EXEC);
            else
                add_compound(d, d->e_list[i], args, EXECP);
            i += index;
        }
        d->cl_size++;
    }
    return 0;
}

void build_ast(struct ast *ast)
{
    // If no expression, build ast with -print
    if (!ast->cl_size)
        return;
    // Search first top level OR
    // cl_size > 2 because OR and AND bound 2 other expressions
    if (ast->cl_size > 2 && (search_OR_AND(ast, OR) || search_OR_AND(ast, AND)))
    {
        build_ast(ast->left);
        build_ast(ast->right);
        return;
    }

    // Make a THEN
    struct compound **c_list_1;
    struct compound **c_list_2;
    ast->et = THEN;
    if (ast->c_list[0]->et != PAO)
    {
        c_list_1 = calloc(1, sizeof(struct compound *));
        c_list_1[0] = ast->c_list[0];
        ast->left = calloc(1, sizeof(struct ast));
        ast->left->c_list = c_list_1;
        ast->left->et = ast->c_list[0]->et;
        ast->left->cl_size = 1;
        if (ast->cl_size > 1)
        {
            c_list_2 = calloc(ast->cl_size - 1, sizeof(struct compound *));
            for (size_t j = 1; j < ast->cl_size; j++)
                c_list_2[j - 1] = ast->c_list[j];
            ast->right = calloc(1, sizeof(struct ast));
            ast->right->c_list = c_list_2;
            ast->right->cl_size = ast->cl_size - 1;
            build_ast(ast->right);
        }
        else
            ast->right = NULL;
        return;
    }
    // ELSE
    int k = find_close(ast, 0);
    size_t i = k;
    c_list_1 = calloc(i - 1, sizeof(struct compound *));
    for (size_t j = 1; j < i; j++)
        c_list_1[j - 1] = ast->c_list[j];
    ast->left = calloc(1, sizeof(struct ast));
    ast->left->c_list = c_list_1;
    ast->left->cl_size = i - 1;
    build_ast(ast->left);
    if (ast->cl_size - i - 1 > 0)
    {
        ast->right = calloc(1, sizeof(struct ast));
        c_list_2 = calloc(ast->cl_size - i - 1, sizeof(struct compound *));
        for (size_t j = i + 1; j < ast->cl_size; j++)
            c_list_2[j - i - 1] = ast->c_list[j];
        ast->right->c_list = c_list_2;
        ast->right->cl_size = ast->cl_size - i - 1;
        build_ast(ast->right);
    }
}

int exec_ast(struct ast *parent, struct ast *ast, struct node *n, int child)
{
    int status = 0;
    switch (ast->et)
    {
    case THEN:
        if (ast->left->et == NO)
        {
            if (exec_ast(ast, ast->right, n, 1))
            {
                parent->rvalue[child] = 0;
                return 0;
            }
            else
            {
                parent->rvalue[0] = 1;
                parent->rvalue[1] = 1;
                return 1;
            }
        }
        else if (exec_ast(ast, ast->left, n, 0))
        {
            if (ast->right)
                exec_ast(ast, ast->right, n, 1);
            else
                ast->rvalue[1] = 1;
        }
        if (ast->rvalue[0] == 1 && ast->rvalue[1] == 1)
        {
            parent->rvalue[child] = 1;
            return 1;
        }
        else
        {
            parent->rvalue[child] = 0;
            return 0;
        }
        break;
    case AND:
        if (exec_ast(ast, ast->left, n, 0) && exec_ast(ast, ast->right, n, 1))
        {
            parent->rvalue[child] = 1;
            return 1;
        }
        else
        {
            parent->rvalue[child] = 0;
            return 1;
        }
        break;
    case OR:
        if (exec_ast(ast, ast->left, n, 0) || exec_ast(ast, ast->right, n, 1))
        {
            parent->rvalue[child] = 1;
            return 1;
        }
        else
        {
            parent->rvalue[child] = 0;
            return 1;
        }
        break;
    case PRINT:
        printf("%s\n", n->name);
        parent->rvalue[child] = 1;
        return 1;
        break;
    case CONDITION:
        if ((my_strcmp("-name", ast->c_list[0]->name) == 0 &&
             !fnmatch(ast->c_list[0]->args[0], n->name_wp, 0)) ||
            (my_strcmp("-type", ast->c_list[0]->name) == 0 &&
             ((my_strcmp("b", ast->c_list[0]->args[0]) == 0 && S_ISBLK(n->type)) ||
              (my_strcmp("c", ast->c_list[0]->args[0]) == 0 && S_ISCHR(n->type)) ||
              (my_strcmp("d", ast->c_list[0]->args[0]) == 0 && S_ISDIR(n->type)) ||
              (my_strcmp("f", ast->c_list[0]->args[0]) == 0 && S_ISREG(n->type)) ||
              (my_strcmp("l", ast->c_list[0]->args[0]) == 0 && S_ISLNK(n->type)) ||
              (my_strcmp("p", ast->c_list[0]->args[0]) == 0 && S_ISFIFO(n->type)) ||
              (my_strcmp("s", ast->c_list[0]->args[0]) == 0 && S_ISSOCK(n->type)))))
        {
            parent->rvalue[child] = 1;
            return 1;
        }
        else if (my_strcmp("-perm", ast->c_list[0]->name) == 0)
        {
            int m = n->type & (S_IRWXU | S_IRWXG | S_IRWXO);
            if (!fnmatch("???", ast->c_list[0]->args[0], 0))
            {
                int arg_mod = my_stroi(ast->c_list[0]->args[0], 0);
                int arg_dec = octal_to_dec(arg_mod);
                if (m == arg_dec)
                {
                    parent->rvalue[child] = 1;
                    return 1;
                }
                else
                {
                    parent->rvalue[child] = 0;
                    return 0;
                }
            }
            else
            {
                parent->rvalue[child] = 0;
                return 0;
            }
        }
        else
        {
            parent->rvalue[child] = 0;
            return 0;
        }
        break;
    case EXECP:
    case EXEC:
        if (ast->cl_size > 0)
        {
            size_t i = 0;
            while (ast->c_list[0]->args[i] != NULL)
                i++;
            char **new_args = calloc(i + 1, sizeof(char *));
            for (size_t j = 0; j < i; j++)
                if (brackets_finder(ast->c_list[0]->args[j]) == 0)
                    new_args[j] = replace_echo(ast->c_list[0]->args[j], n->name);
                else
                    new_args[j] = my_strcp(ast->c_list[0]->args[j]);
            pid_t pid = fork();
            if (pid == 0) // child
            {
                execvp(new_args[0], new_args); // foo should appear on stdout
                fprintf(stderr, "An error occured while execvp\n");
                exit(1);
            }
            else // father
            {
                waitpid(pid, &status, 0);
                for (size_t j = 0; j < i; j++)
                    free(new_args[j]);
                free(new_args);
            }
        }
        return status;
        break;
    default:
        return 0;
        break;
    }
    return 0;
}

void my_realloc(struct data *d, int id)
{
    if (id == 0)
    {
        d->nl_capacity *= 2;
        d->name_list = realloc(d->name_list, d->nl_capacity * sizeof(char *));
    }
    else if (id == 1)
    {
        d->el_capacity *= 2;
        d->e_list = realloc(d->e_list, d->el_capacity * sizeof(char *));
    }
    else if (id == 2)
    {
        d->no_capacity *= 2;
        d->nodes = realloc(d->nodes, d->no_capacity * sizeof(struct node *));
    }
    else if (id == 3)
    {
        d->il_capacity *= 2;
        d->inode_list = realloc(d->inode_list, d->il_capacity * sizeof(int));
    }
    else if (id == 4)
    {
        d->cl_capacity *= 2;
        d->c_list = realloc(d->c_list, d->cl_capacity * sizeof(struct compound *));
    }
}

void add_search_path(struct data *d, char *name)
{
    if (d->nl_size < d->nl_capacity)
    {
        d->name_list[d->nl_size] = my_strcp(name);
        d->nl_size++;
    }
    else
    {
        my_realloc(d, 0);
        add_search_path(d, name);
    }
}

void add_exp(struct data *d, char *exp)
{
    if (d->el_size < d->el_capacity)
    {
        d->e_list[d->el_size] = exp;
        d->el_size++;
    }
    else
    {
        my_realloc(d, 1);
        add_exp(d, exp);
    }
}

void add_inode(int ino, struct data *d)
{
    if (d->il_size < d->il_capacity)
    {
        d->inode_list[d->il_size] = ino;
        d->il_size++;
    }
    else
    {
        my_realloc(d, 3);
        add_inode(ino, d);
    }
}

int inode_exists(int ino, struct data *d)
{
    for (size_t i = 0; i < d->il_size; i++)
        if (d->inode_list[i] == ino)
            return 1;
    return 0;
}

void add_node(char *name, char *name_wp, mode_t type, mode_t r_type,
              struct data *d)
{
    struct node *n = calloc(sizeof(struct node), 1);
    n->name = name;
    n->name_wp = name_wp;
    n->type = type;
    n->r_type = r_type;
    if (d->no_size >= d->no_capacity)
        my_realloc(d, 2);
    d->nodes[d->no_size] = n;
    d->no_size++;
}

void add_compound(struct data *d, char *name, char **args, enum enum_type et)
{
    struct compound *c = calloc(1, sizeof(struct compound));
    c->name = name;
    c->args = args;
    c->et = et;
    if (d->cl_size >= d->cl_capacity)
        my_realloc(d, 4);
    d->c_list[d->cl_size] = c;
}

void parse_dir(char *name, struct data *d)
{
    DIR *di = opendir(name);
    struct dirent *dir;
    struct stat sb;  // 用于保存符号链接指向的目标文件的信息
    struct stat sbl; // 用于保存符号链接本身的信息
    mode_t types[2]; // 存储文件和符号链接的类型
    int islnk;       // 记录文件是否是符号链接
    char *new_name;
    while ((dir = readdir(di)) != NULL)
    {
        if (my_strcmp(dir->d_name, ".") == 0 || my_strcmp(dir->d_name, "..") == 0)
            continue;
        new_name = my_concate(name, dir->d_name);
        lstat(new_name, &sbl); // 获取符号链接本身的信息
        stat(new_name, &sb);   // 获取的是符号链接指向的目标文件的信息
        islnk = S_ISLNK(sbl.st_mode);
        types[0] = sbl.st_mode;
        types[1] = sb.st_mode;

        add_node(new_name, my_strcp(dir->d_name), types[0], types[1], d);

        // 如果不是目录，直接跳过
        if (!S_ISDIR(sb.st_mode))
            continue;
        // 如果是目录，检查是否符号链接或选项允许递归解析
        else if (!islnk || d->option == 2)
        {
            // 检查是否已解析过该inode
            if (inode_exists(sb.st_ino, d))
            {
                d->return_value = 1;
                continue;
            }
            add_inode(sb.st_ino, d);
            parse_dir(new_name, d);
        }
    }
    free(di);
}

int find_close(struct ast *ast, int i)
{
    int pa_count = 1;
    while (ast->c_list[i]->et != PAC || pa_count != 0)
    {
        i++;
        if (ast->c_list[i]->et == PAO)
            pa_count++;
        else if (ast->c_list[i]->et == PAC)
            pa_count--;
    }
    return i;
}

int search_OR_AND(struct ast *ast, unsigned int type)
{
    struct compound **c_list_1;
    struct compound **c_list_2;
    // 1 and cl_size - 1 because can't be first or last element
    for (size_t i = 0; i < ast->cl_size - 1; i++)
    {
        if ((ast->c_list[i]->et == type))
        {
            ast->et = type;
            c_list_1 = calloc(i, sizeof(struct compound *));
            c_list_2 = calloc(ast->cl_size - i, sizeof(struct compound *));
            for (size_t j = 0; j < i; j++)
                c_list_1[j] = ast->c_list[j];
            for (size_t j = i + 1; j < ast->cl_size; j++)
                c_list_2[j - i - 1] = ast->c_list[j];
            ast->left = calloc(1, sizeof(struct ast));
            ast->right = calloc(1, sizeof(struct ast));
            ast->left->c_list = c_list_1;
            ast->right->c_list = c_list_2;
            ast->left->cl_size = i;
            ast->right->cl_size = ast->cl_size - i - 1;
            return 1;
        }
        else if (ast->c_list[i]->et == PAO)
            i += find_close(ast, i);
    }
    return 0;
}

char *replace_echo(char *str, char *name)
{
    size_t str_s = my_strlen(str);
    size_t name_s = my_strlen(name);
    char *new_str = calloc(str_s + name_s + 1, sizeof(char));
    size_t i = 0;
    size_t index = 0;
    while (str[i])
    {
        if (str[i] == '{' && str + 1 && str[i + 1] == '}')
        {
            for (size_t j = 0; j < name_s; j++)
            {
                new_str[i + j] = name[j];
                index++;
            }
            i++;
        }
        else
        {
            new_str[index] = str[i];
            index++;
        }
        i++;
    }
    return new_str;
}

int brackets_finder(char *str)
{
    size_t l = my_strlen(str);
    l = (l == 0) ? 0 : l - 1;
    for (size_t i = 0; i < l; i++)
        if (str[i] == '{' && str[i + 1] == '}')
            return 0;
    return 1;
}

int is_ast_valid(struct data *d, struct ast *parent, struct ast *ast, int child)
{
    if (!ast)
        return 0;
    int rvalue = 0;
    switch (ast->et)
    {
    case THEN:
        if (!ast->left && parent)
            rvalue = 1;
        break;
    case AND:
    case OR:
        if (!(ast->left && ast->right))
            rvalue = 1;
        break;
    case NO:
        if (child || !parent->right)
            rvalue = 1;
        break;
    case PRINT:
    case EXEC:
        d->actions = 1;
    default:
        break;
    }
    if (!rvalue)
        return is_ast_valid(d, ast, ast->left, 0) +
               is_ast_valid(d, ast, ast->right, 1);
    return 1;
}

void invert_node_list(struct data *d)
{
    struct node **new_nodes = calloc(d->no_capacity, sizeof(struct node *));
    for (size_t i = 0; i < d->no_size; i++)
        new_nodes[i] = d->nodes[d->no_size - i - 1];
    free(d->nodes);
    d->nodes = new_nodes;
}

void reset_rvalues(struct ast *root)
{
    if (!root)
        return;
    root->rvalue[0] = 1;
    root->rvalue[1] = 1;
    reset_rvalues(root->left);
    reset_rvalues(root->right);
}

void free_il(struct data *d)
{
    free(d->inode_list);
    d->inode_list = calloc(10, sizeof(int));
    d->il_size = 0;
    d->il_capacity = 10;
}

void free_ast(struct ast *ast)
{
    if (ast)
    {
        free_ast(ast->left);
        free_ast(ast->right);
        free(ast->c_list);
        free(ast);
    }
}

void free_data(struct data *d)
{
    for (size_t i = 0; i < d->no_size; i++)
    {
        free(d->nodes[i]->name);
        free(d->nodes[i]->name_wp);
        free(d->nodes[i]);
    }
    free(d->nodes);
    for (size_t i = 0; i < d->el_size; i++)
        free(d->e_list[i]);
    free(d->e_list);
    free(d->inode_list);
    free(d->name_list);
    for (size_t i = 0; i < d->cl_size; i++)
    {
        free(d->c_list[i]->args);
        free(d->c_list[i]);
    }
    free_ast(d->ast);
}

void print_ast(struct ast *ast, int i, int side)
{
    if (!ast)
        return;
    printf("%i | %i) ", i, side);
    switch (ast->et)
    {
    case THEN:
        printf("THEN ");
        break;
    case AND:
        printf("AND ");
        break;
    case OR:
        printf("OR ");
        break;
    case NO:
        printf("NO ");
        break;
    case PRINT:
        printf("PRINT ");
        break;
    case CONDITION:
        printf("condition ");
        break;
    case EXECP:
    case EXEC:
        printf("EXEC ");
        break;
    case PAO:
        printf("PAO ");
        break;
    case PAC:
        printf("PAC ");
        break;
    case FAPA:
        printf("FAPA ");
        break;
    }
    printf("\n");
    print_ast(ast->left, i + 1, 0);
    print_ast(ast->right, i + 1, 1);
}