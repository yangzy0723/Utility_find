#include "myfind.h"
#include "lib/lib_str.h"
#include "lib/lib_util.h"

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_BATCH_SIZE 4096

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
    if (d.spl_size == 0)
    {
        d.spl_size = 1;
        d.search_path_list[0] = my_strcp(".");
    }
    // 解析表达式
    for (; index < argc; index++)
    {
        char *exp = my_strcp(argv[index]);
        add_exp(&d, exp);
    }
    // 创建命令列表并检查错误
    if (create_c_list(&d))
    {
        int rv = d.return_value;
        d.ast->c_list = d.c_list;
        free_data(&d);
        return rv;
    }
    // 创建抽象语法树
    d.ast->c_list = d.c_list;
    d.ast->cl_size = d.cl_size;
    build_ast(d.ast);
    reset_rvalues(d.ast);
    // 检查抽象语法树
    if (is_ast_valid(&d, NULL, d.ast, 0))
    {
        free(d.c_list);
        free_data(&d);
        fprintf(stderr, "Expressions error\n");
        return 1;
    }

    generate_nodes(&d);
    if (d.bfl_size)
        d.return_value = deal_batch_remaining(&d);
    // for (int i = 0; i < d.no_size; i++)
    // {
    //     if (d.ast->left)
    //         exec_ast(d.ast, d.ast, d.nodes[i], 0);
    //     if (!d.actions && d.ast->rvalue[0] == 1 && d.ast->rvalue[1] == 1)
    //         printf("%s\n", d.nodes[i]->name);
    //     reset_rvalues(d.ast);
    // }
    int rvalue = 0;
    rvalue = d.return_value;
    free_data(&d);
    return (rvalue);
}

void init_data(struct data *d)
{
    d->option = 0;
    d->return_value = 0;
    d->d_checked = 0;
    d->search_path_list = calloc(10, sizeof(char *));
    d->exp_list = calloc(10, sizeof(char *));
    d->nodes = calloc(10, sizeof(struct node *));
    d->inode_list = calloc(10, sizeof(int));
    d->c_list = calloc(10, sizeof(struct compound *));
    d->batch_file_list = calloc(10, sizeof(char *));
    d->spl_size = 0;
    d->el_size = 0;
    d->no_size = 0;
    d->il_size = 0;
    d->cl_size = 0;
    d->bfl_size = 0;
    d->spl_capacity = 10;
    d->el_capacity = 10;
    d->no_capacity = 10;
    d->il_capacity = 10;
    d->cl_capacity = 10;
    d->bfl_capacity = 10;
    d->actions = 0;
    d->ast = calloc(1, sizeof(struct ast));
    d->ast->left = NULL;
    d->ast->right = NULL;
}

int update_option(struct data *d, char *opt)
{
    // 启用深度优先搜索
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
    int srl;         // 存储lstat()的返回值
    int islnk;       // 存储该文件是否是符号链接
    for (size_t i = 0; i < d->spl_size; i++)
    {
        // lstat系统调用：lstat会获取符号链接本身的状态信息，而不是符号链接指向的目标文件的信息
        // stat系统调用：stat系统调用用于获取文件或目录的状态信息，如果该文件是一个符号链接，stat会返回符号链接所指向的目标文件的信息，而不是符号链接本身的信息
        srl = lstat(d->search_path_list[i], &sbl);
        if (srl == -1)
        {
            fprintf(stderr, "\'%s\' : No such file or directory\n", d->search_path_list[i]);
            d->return_value = 1;
            continue;
        }
        stat(d->search_path_list[i], &sb);
        islnk = S_ISLNK(sbl.st_mode);
        types[0] = sbl.st_mode;
        types[1] = sb.st_mode;
        // 深度优先搜索
        if (d->d_checked)
        {
            if (S_ISDIR(sb.st_mode) && (!islnk || d->option == 1 || d->option == 2))
            {
                add_inode(d, sb.st_ino);
                parse_dir(d->search_path_list[i], d);
            }
            // 传入的name_wp应该是目录/文件名，不包含路径
            if (my_strcmp(d->search_path_list[i], ".") == 0 || my_strcmp(d->search_path_list[i], "..") == 0 || my_strcmp(d->search_path_list[i], "/") == 0)
                add_node(d->search_path_list[i], my_strcp(d->search_path_list[i]), types[0], types[1], d);
            else
            {
                char *last_slash = my_strrchr(d->search_path_list[i], '/');
                char *f_name = my_strcp(last_slash + 1);
                add_node(d->search_path_list[i], f_name, types[0], types[1], d);
            }
            free_il(d);
        }
        // 广度优先搜索
        else
        {
            // 传入的name_wp应该是目录/文件名，不包含路径
            if (my_strcmp(d->search_path_list[i], ".") == 0 || my_strcmp(d->search_path_list[i], "..") == 0 || my_strcmp(d->search_path_list[i], "/") == 0)
                add_node(d->search_path_list[i], my_strcp(d->search_path_list[i]), types[0], types[1], d);
            else
            {
                char *last_slash = my_strrchr(d->search_path_list[i], '/');
                char *f_name = my_strcp(last_slash + 1);
                add_node(d->search_path_list[i], f_name, types[0], types[1], d);
            }
            if (S_ISDIR(sb.st_mode) && (!islnk || d->option == 1 || d->option == 2))
            {
                add_inode(d, sb.st_ino);
                parse_dir(d->search_path_list[i], d);
            }
            free_il(d);
        }
    }
}

int deal_batch_remaining(struct data *d) {
    char **args_batch = calloc(d->bfl_size + 2, sizeof(char *));
    int ret = 0;
    // TODO: 如果需要批处理，必然有-exec，此时c_list[0]->args[0]就是批处理命令
    args_batch[0] = my_strcp(d->batch_command);
    for (int i = 0; i < d->bfl_size; i++)
        args_batch[i + 1] = my_strcp(d->batch_file_list[i]);
    args_batch[d->bfl_size + 1] = NULL;
    pid_t pid = fork();
    // child
    if (pid == 0)
    {
        execvp(args_batch[0], args_batch);
        fprintf(stderr, "An error occured while execvp\n");
        exit(1);
    }
    // father
    else
    {
        waitpid(pid, &ret, 0);
        for (size_t i = 0; i < d->bfl_size + 2; i++)
            free(args_batch[i]);
        free(args_batch);
    }
    free_bfl(d);
    return ret;    
}

int create_c_list(struct data *d)
{
    char **args;
    for (size_t i = 0; i < d->el_size; i++)
    {
        if (my_strcmp("-print", d->exp_list[i]) == 0)
            add_compound(d, d->exp_list[i], NULL, PRINT);
        else if (my_strcmp("-a", d->exp_list[i]) == 0)
            add_compound(d, d->exp_list[i], NULL, AND);
        else if (my_strcmp("-o", d->exp_list[i]) == 0)
            add_compound(d, d->exp_list[i], NULL, OR);
        else if (my_strcmp("!", d->exp_list[i]) == 0)
            add_compound(d, d->exp_list[i], NULL, NO);
        else if (my_strcmp("(", d->exp_list[i]) == 0)
            add_compound(d, d->exp_list[i], NULL, PAO);
        else if (my_strcmp(")", d->exp_list[i]) == 0)
            add_compound(d, d->exp_list[i], NULL, PAC);
        else if (my_strcmp("-type", d->exp_list[i]) == 0 ||
                 my_strcmp("-name", d->exp_list[i]) == 0 ||
                 my_strcmp("-perm", d->exp_list[i]) == 0)
        {
            if (i >= d->el_size - 1)
            {
                d->return_value = 1;
                fprintf(stderr, "Invalid condition syntaxe\n");
                return 1;
            }
            args = calloc(2, sizeof(char *));
            args[0] = d->exp_list[i + 1];
            add_compound(d, d->exp_list[i], args, CONDITION);
            i++;
        }
        else
        {
            size_t index = 1;
            // Need handle wrong commands
            while (my_strcmp(";", d->exp_list[i + index]) && my_strcmp("+", d->exp_list[i + index]))
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
                args[j] = d->exp_list[i + j + 1];
            if (my_strcmp(";", d->exp_list[index + i]) == 0)
                add_compound(d, d->exp_list[i], args, EXEC);
            else
                add_compound(d, d->exp_list[i], args, EXECP);
            i += index;
        }
        d->cl_size++;
    }
    return 0;
}

void build_ast(struct ast *ast)
{
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

int exec_ast(struct data *d, struct ast *parent, struct ast *ast, struct node *n, int child)
{
    int status = 0;
    switch (ast->et)
    {
    case THEN:
        if (ast->left->et == NO)
        {
            if (exec_ast(d, ast, ast->right, n, 1))
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
        else if (exec_ast(d, ast, ast->left, n, 0))
        {
            if (ast->right)
                exec_ast(d, ast, ast->right, n, 1);
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
        if (exec_ast(d, ast, ast->left, n, 0) && exec_ast(d, ast, ast->right, n, 1))
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
        if (exec_ast(d, ast, ast->left, n, 0) || exec_ast(d, ast, ast->right, n, 1))
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
        if (ast->cl_size > 0)
        {
            size_t i = 0;
            while (ast->c_list[0]->args[i] != NULL)
                i++;
            char **new_args = calloc(i + 1, sizeof(char *));
            for (size_t j = 0; j < i; j++)
                if (brackets_finder(ast->c_list[0]->args[j]) == 0)
                    // 进入该if，保证已经存在一对紧密相连的大括号{}
                    new_args[j] = replace_echo(ast->c_list[0]->args[j], n->name);
                else
                    new_args[j] = my_strcp(ast->c_list[0]->args[j]);\
            // 用于记录exec命令，用于处理未到达批处理临界的最后一批次数据
            d->batch_command = my_strcp(new_args[0]);
            for (size_t j = 1; j < i; j++)
            {
                // 考虑到垃圾回收机制，字符串拷贝时需要使用my_strcp，而不能直接赋值
                if (!add_batch_file(d, new_args[1]))
                {
                    char **new_args_batch = calloc(d->bfl_size + 2, sizeof(char *));
                    new_args_batch[0] = my_strcp(new_args[0]);
                    for (int k = 1; k <= d->bfl_size; k++)
                        new_args_batch[k] = my_strcp(d->batch_file_list[k - 1]);
                    new_args_batch[d->bfl_size + 1] = NULL;
                    pid_t pid = fork();
                    // child
                    if (pid == 0)
                    {
                        execvp(new_args_batch[0], new_args_batch);
                        fprintf(stderr, "An error occured while execvp\n");
                        exit(1);
                    }
                    // father
                    else
                    {
                        waitpid(pid, &status, 0);
                        for (size_t k = 0; k < d->bfl_size + 2; k++)
                            free(new_args_batch[k]);
                        free(new_args_batch);
                    }
                    free_bfl(d);
                    add_batch_file(d, new_args[i]);
                }
            }
            for (size_t j = 0; j < i; j++)
                free(new_args[j]);
            free(new_args);
        }
        return status;
        break;
    case EXEC:
        if (ast->cl_size > 0)
        {
            size_t i = 0;
            while (ast->c_list[0]->args[i] != NULL)
                i++;
            char **new_args = calloc(i + 1, sizeof(char *));
            for (size_t j = 0; j < i; j++)
                if (brackets_finder(ast->c_list[0]->args[j]) == 0)
                    // 进入该if，保证已经存在一对紧密相连的大括号{}
                    new_args[j] = replace_echo(ast->c_list[0]->args[j], n->name);
                else
                    new_args[j] = my_strcp(ast->c_list[0]->args[j]);
            pid_t pid = fork();
            // child
            if (pid == 0)
            {
                execvp(new_args[0], new_args);
                fprintf(stderr, "An error occured while execvp\n");
                exit(1);
            }
            // father
            else
            {
                waitpid(pid, &status, 0);
                for (size_t j = 0; j < i + 1; j++)
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
        d->spl_capacity *= 2;
        d->search_path_list = realloc(d->search_path_list, d->spl_capacity * sizeof(char *));
    }
    else if (id == 1)
    {
        d->el_capacity *= 2;
        d->exp_list = realloc(d->exp_list, d->el_capacity * sizeof(char *));
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
    else if (id == 5)
    {
        d->bfl_capacity *= 2;
        d->batch_file_list = realloc(d->batch_file_list, d->bfl_capacity * sizeof(char *));
    }
}

void add_search_path(struct data *d, char *name)
{
    if (d->spl_size < d->spl_capacity)
    {
        d->search_path_list[d->spl_size] = my_strcp(name);
        d->spl_size++;
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
        d->exp_list[d->el_size] = exp;
        d->el_size++;
    }
    else
    {
        my_realloc(d, 1);
        add_exp(d, exp);
    }
}

void add_inode(struct data *d, int ino)
{
    if (d->il_size < d->il_capacity)
    {
        d->inode_list[d->il_size] = ino;
        d->il_size++;
    }
    else
    {
        my_realloc(d, 3);
        add_inode(d, ino);
    }
}

int inode_exists(struct data *d, int ino)
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
    if (d->ast->left)
        exec_ast(d, d->ast, d->ast, n, 0);
    if (!d->actions && d->ast->rvalue[0] == 1 && d->ast->rvalue[1] == 1)
        printf("%s\n", n->name);
    reset_rvalues(d->ast);
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

int add_batch_file(struct data *d, char *batch_file)
{
    if (d->bfl_size < d->bfl_capacity)
    {
        d->batch_file_list[d->bfl_size] = my_strcp(batch_file);
        d->bfl_size++;
        return 1;
    }
    else
    {
        if (d->bfl_size >= MAX_BATCH_SIZE)
            return 0;
        else
        {
            my_realloc(d, 5);
            add_batch_file(d, batch_file);
            return 1;
        }
    }
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
        // 深度优先搜索
        if (d->d_checked)
        {
            // 如果是目录，检查是否符号链接或选项允许递归解析
            if (S_ISDIR(sb.st_mode) && (!islnk || d->option == 2))
            {
                // 检查是否已解析过该inode
                if (inode_exists(d, sb.st_ino))
                    d->return_value = 1;
                else
                {
                    add_inode(d, sb.st_ino);
                    parse_dir(new_name, d);
                }
            }
            add_node(new_name, my_strcp(dir->d_name), types[0], types[1], d);
        }
        // 广度优先搜索
        else
        {
            add_node(new_name, my_strcp(dir->d_name), types[0], types[1], d);
            // 如果是目录，检查是否符号链接或选项允许递归解析
            if (S_ISDIR(sb.st_mode) && (!islnk || d->option == 2))
            {
                // 检查是否已解析过该inode
                if (inode_exists(d, sb.st_ino))
                    d->return_value = 1;
                else
                {
                    add_inode(d, sb.st_ino);
                    parse_dir(new_name, d);
                }
            }
        }
    }
    closedir(di);
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
    size_t index = 0;
    char *new_str = calloc(str_s + name_s + 1, sizeof(char));
    for (int i = 0; i < str_s; i++)
    {
        if (i < str_s - 1 && str[i] == '{' && str[i + 1] == '}')
        {
            for (int j = 0; j < name_s; j++)
            {
                new_str[index] = name[j];
                index++;
            }
            i++;
        }
        else
        {
            new_str[index] = str[i];
            index++;
        }
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
    switch (ast->et)
    {
    case THEN:
        if (!ast->left && parent)
            return 1;
        break;
    case AND:
    case OR:
        if (!(ast->left && ast->right))
            return 1;
        break;
    case NO:
        if (child || !parent->right)
            return 1;
        break;
    case PRINT:
    case EXEC:
    case EXECP:
        d->actions = 1;
        break;
    default:
        break;
    }
    return is_ast_valid(d, ast, ast->left, 0) + is_ast_valid(d, ast, ast->right, 1);
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

void free_bfl(struct data *d)
{
    for (int i = 0; i < d->bfl_size; i++)
        free(d->batch_file_list[i]);
    free(d->batch_file_list);
    d->batch_file_list = calloc(10, sizeof(char *));
    d->bfl_size = 0;
    d->bfl_capacity = 10;
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
        free(d->exp_list[i]);
    free(d->exp_list);
    
    free(d->inode_list);
    
    for (size_t i = 0; i < d->spl_size; i++)
        free(d->search_path_list[i]);
    free(d->search_path_list);
    
    free(d->batch_file_list);
    
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