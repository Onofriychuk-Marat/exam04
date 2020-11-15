
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void     ft_putstr(char *str, int fd_write)
{
    if (str == NULL)
        return ;
    while (*str)
        write(fd_write, str++, 1);
}

int		ft_strcmp(char *str1, char *str2)
{
	size_t	i;

	i = 0;
	while ((str1[i] || str2[i]))
	{
		if (str1[i] != str2[i])
			return (str1[i] - str2[i]);
		i++;
	}
	return (0);
}

void    ft_error(char *message, char *command, int fd_write)
{
    ft_putstr(message, fd_write);
    ft_putstr(command, fd_write);
    ft_putstr("\n", fd_write);
}

void    ft_execve(char **argv, char **env, int fd_write)
{
    if (execve(*argv, argv, env) == -1)
        ft_error("error: cannot execute ", *argv, fd_write);
}

void    ft_cmd(char **argv, char **env, int ispipe, int fd_write)
{
    int pid;
    int status;

    if (ispipe)
    {
        ft_execve(argv, env, fd_write);
        return ;
    }
    if ((pid = fork()) == 0)
        ft_execve(argv, env, fd_write);
    else
        waitpid(pid, &status, 0);
}

void    ft_cd(char *path, int count_path, int fd_write)
{
    if (count_path > 1)
        ft_error("error: cd: bad arguments", NULL, fd_write);
    else if (chdir(path) < 0)
		ft_error("error: cd: cannot change directory to ", path, fd_write);
}

void    denouement_cmd(int argc, char **argv, char **env, int ispipe, int fd_write)
{
    if (!ft_strcmp(*argv, "cd"))
        ft_cd(*(argv + 1), argc, fd_write);
    else
        ft_cmd(argv, env, ispipe, fd_write);
}

int     find_pipe(char **argv)
{
    int i;

    i = -1;
    while (argv[++i])
        if (!ft_strcmp(argv[i], "|"))
            return (i);
    return (i);
}

int     get_count_pipe(char **argv)
{
    int i;
    int count;

    i = 0;
    count = 0;
    while (argv[i])
        if (!ft_strcmp(argv[i++], "|"))
            count++;
    return (count > 0 ? count + 1 : 0);
}

void	wait_child(pid_t *children, int **pipefds, int count_children)
{
	int		i;
	int		status;

	i = -1;
	while (++i < count_children)
	{
		waitpid(children[i], &status, 0);
		if (i + 1 != count_children && pipefds[i])
			free(pipefds[i]);
	}
	free(pipefds);
	free(children);
}

void	close_fd(int **pipefds, int i)
{
	if (i > 0)
	{
		close(pipefds[i - 1][0]);
		close(pipefds[i - 1][1]);
	}
}

void	edit_fd(int **pipefds, int i, int count_children)
{
	if (i > 0)
	{
		close(pipefds[i - 1][1]);
		dup2(pipefds[i - 1][0], 0);
	}
	if (i + 1 != count_children)
	{
		close(pipefds[i][0]);
		dup2(pipefds[i][1], 1);
	}
}

void	add_pipe(int **pipefds, int i, int count_children)
{
    if (i + 1 != count_children)
	{
		pipefds[i] = (int*)malloc(sizeof(int) * 2);
		pipe(pipefds[i]);
	}
}

void     start_pipe(char **argv, char **env, int count_children, int fd_write)
{
    int     point_pipe;
	int		i;
	pid_t	*children;
	int		**pipefds;
    
	children = (pid_t*)malloc(sizeof(pid_t) * count_children);
	pipefds = (int**)malloc(sizeof(int*) * (count_children - 1));
	i = -1;
	while (++i < count_children)
	{
        add_pipe(pipefds, i, count_children);
        if ((children[i] = fork()) == 0)
        {
            edit_fd(pipefds, i, count_children);
            point_pipe = find_pipe(argv);
            argv[point_pipe] = NULL;
            denouement_cmd(point_pipe, argv, env, 1, fd_write);
            break ;
        }
        else if (i + 1 != count_children)
            argv += find_pipe(argv) + 1;
        close_fd(pipefds, i);
	}
    wait_child(children, pipefds, count_children);
}

int     main(int argc, char **argv, char **env)
{
    int     i;
    int     point_start;
    int     count_pipe;
    int     fd_write;

    fd_write = dup(1);
    i = 0;
    point_start = 1;
    while (i < argc)
    {
        if (!argv[i + 1] || !ft_strcmp(argv[i + 1], ";"))
        {
            argv[i + 1] = NULL;
            if (i + 1 > point_start && !(count_pipe = get_count_pipe(argv + point_start)))
                denouement_cmd(i - point_start, argv + point_start, env, 0, fd_write);
            else if (i + 1 > point_start)
                start_pipe(argv + point_start, env, count_pipe, fd_write);
            point_start = i + 2;
        }
        i++;
    }
    return (0);
}
