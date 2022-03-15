#include <string>
#include <fstream>
#include <sys/stat.h>
#include <fts.h>
#include <sys/types.h>
#include <string.h>

class FileTestUtil
{
public:
    static bool write_file(const std::string &file_name, const std::string& body)
    {
        std::ofstream example_file;
        example_file.open(file_name, std::ios::out | std::ios::trunc);
        if (example_file.is_open())
        {
            example_file << body;
        } else 
        {
            return false;
        }
        example_file.close();
        return true;
    }

    static int create_dir(const char *path, mode_t mode)
    {
        struct stat st;
        int status = 0;
        if(stat(path, &st) != 0)
        {
            // Directory does not exist. EEXIST for race condition
            if(::mkdir(path, mode) != 0 && errno != EEXIST)
                status = -1;
        }
        else if(!S_ISDIR(st.st_mode))
        {
            errno = ENOTDIR;
            status = -1;
        }
        return status;
    }

    // mkpath - ensure all directories in path exist
    static int mkpath(const char *path, mode_t mode)
    {
        char *pp;
        char *sp;
        int  status;
        char *copypath = strdup(path);

        status = 0;
        pp = copypath;
        while (status == 0 && (sp = strchr(pp, '/')) != 0)
        {
            if (sp != pp)
            {
                // Neither root nor double slash in path
                *sp = '\0';
                status = create_dir(copypath, mode);
                *sp = '/';
            }
            pp = sp + 1;
        }
        if (status == 0)
            status = create_dir(path, mode);
        free(copypath);
        return (status);
    }

    // https://stackoverflow.com/questions/2256945/removing-a-non-empty-directory-programmatically-in-c-or-c
    static int recursive_delete(const char *dir)
    {
        int ret = 0;
        FTS *ftsp = NULL;
        FTSENT *curr;

        // Cast needed (in C) because fts_open() takes a "char * const *", instead
        // of a "const char * const *", which is only allowed in C++. fts_open()
        // does not modify the argument.
        char *files[] = { (char *) dir, NULL };

        // FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
        //                in multithreaded programs
        // FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
        //                of the specified directory
        // FTS_XDEV     - Don't cross filesystem boundaries
        ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
        if (!ftsp) {
            fprintf(stderr, "%s: fts_open failed: %s\n", dir, strerror(errno));
            ret = -1;
            goto finish;
        }

        while ((curr = fts_read(ftsp))) {
            switch (curr->fts_info) {
            case FTS_NS:
            case FTS_DNR:
            case FTS_ERR:
                fprintf(stderr, "%s: fts_read error: %s\n",
                        curr->fts_accpath, strerror(curr->fts_errno));
                break;

            case FTS_DC:
            case FTS_DOT:
            case FTS_NSOK:
                // Not reached unless FTS_LOGICAL, FTS_SEEDOT, or FTS_NOSTAT were
                // passed to fts_open()
                break;

            case FTS_D:
                // Do nothing. Need depth-first search, so directories are deleted
                // in FTS_DP
                break;

            case FTS_DP:
            case FTS_F:
            case FTS_SL:
            case FTS_SLNONE:
            case FTS_DEFAULT:
                if (remove(curr->fts_accpath) < 0) {
                    fprintf(stderr, "%s: Failed to remove: %s\n",
                            curr->fts_path, strerror(curr->fts_errno));
                    ret = -1;
                }
                break;
            }
        }

    finish:
        if (ftsp) {
            fts_close(ftsp);
        }

        return ret;
    }
};