#include "fs.h"
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"

char *get_bin_dir(char *argv0)
{
	char *tmp, *res;

	check_ptr(tmp = realpath(argv0, NULL));
	res = sstrdup(dirname(tmp));
	free(tmp);
	return res;
}

void smkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res < 0 && errno != EEXIST)
		check_sys(res);
}

void sunlink(const char *path)
{
	int res;

	res = unlink(path);
	if (res < 0 && errno != ENOENT)
		check_sys(res);
}

void ssymlink(const char *oldpath, const char *newpath)
{
	int res;

	res = symlink(oldpath, newpath);
	if (res < 0 && errno == EEXIST) {
		sunlink(newpath);
		res = symlink(oldpath, newpath);
	}
	check_sys(res);
}

static bool strip_nl(char *s)
{
	int len = strlen(s);

	if (len && s[len - 1] == '\n') {
		s[len - 1] = '\0';
		return true;
	}
	return false;
}

char *fgetline(char *s, int size, FILE *stream)
{
	char *res;
	bool consume;

	res = fgets(s, size, stream);
	if (!res)
		return res;
	consume = !strip_nl(s);
	while (consume) {
		char buf[64];

		if (!fgets(buf, sizeof(buf), stream))
			break;
		consume = !strip_nl(buf);
	}
	return res;
}

void close_fds(void)
{
	/* Based on the code in isolate/util.c. Note that we may have
	 * a fd to syslog opened at this point but it seems to be safe to
	 * close it: the vsyslog call will reopen it. */
	DIR *dir;
	struct dirent *e;

	check_ptr(dir = opendir("/proc/self/fd"));
	while ((e = readdir(dir))) {
		long fd;

		if (!to_long(e->d_name, &fd))
			continue;
		if ((fd >= 0 && fd <= 2) || fd == dirfd(dir))
			continue;
		close(fd);
	}
	closedir(dir);
}
