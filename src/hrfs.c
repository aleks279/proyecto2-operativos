 
#include "config.h"
#include "params.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"

// construir un path por si se necesita 

static void hrfs_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, HRFS_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will
				    // break here

    log_msg("    hrfs_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
	    HRFS_DATA->rootdir, path, fpath);
}

///////////////////////////////////////////////////////////
//
//
/** Obtener atributos del archivo 
 *
 *  
 */
int hrfs_getattr(const char *path, struct stat *statbuf)
{
    int retstat;
    char fpath[PATH_MAX];
    
    log_msg("\nhrfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);
    hrfs_fullpath(fpath, path);

    retstat = log_syscall("lstat", lstat(fpath, statbuf), 0);
    
    log_stat(statbuf);
    
    return retstat;
}

/** Crear directorio */
int hrfs_mkdir(const char *path, mode_t mode)
{
    char fpath[PATH_MAX];
    
    log_msg("\nhrfs_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
    hrfs_fullpath(fpath, path);

    return log_syscall("mkdir", mkdir(fpath, mode), 0);
}

/** Eliminar archivo */
int hrfs_unlink(const char *path)
{
    char fpath[PATH_MAX];
    
    log_msg("hrfs_unlink(path=\"%s\")\n",
	    path);
    hrfs_fullpath(fpath, path);

    return log_syscall("unlink", unlink(fpath), 0);
}

/** Eliminar un directorio */
int hrfs_rmdir(const char *path)
{
    char fpath[PATH_MAX];
    
    log_msg("hrfs_rmdir(path=\"%s\")\n",
	    path);
    hrfs_fullpath(fpath, path);

    return log_syscall("rmdir", rmdir(fpath), 0);
}


/** Renombrar archivo */
int hrfs_rename(const char *path, const char *newpath)
{
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];
    
    log_msg("\nhrfs_rename(fpath=\"%s\", newpath=\"%s\")\n",
	    path, newpath);
    hrfs_fullpath(fpath, path);
    hrfs_fullpath(fnewpath, newpath);

    return log_syscall("rename", rename(fpath, fnewpath), 0);
}



/** Abrir archivo */
int hrfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd;
    char fpath[PATH_MAX];
    
    log_msg("\nhrfs_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);
    hrfs_fullpath(fpath, path);
    // En caso de que el abrir funcione el retsrar es el descriptor de archivo si no es -errno
    fd = log_syscall("open", open(fpath, fi->flags), 0);
    if (fd < 0)
	retstat = log_error("open");
	
    fi->fh = fd;

    log_fi(fi);
    
    return retstat;
}

/** Leer datos de un archivo abierto 
 *
 * Read debería de retornar exactamente el número de bytes pedido excepto en EOF, 
 * en otro modo el resto de los datos será sustituidos por cero. 
 * una excepción de esto es cuando la opción mount direct_io es especificada 
 * y retornará ek valor de la llamado al sistema read que será reflejado el valor de retorno de esta operación 
 * 
 * 
 */
int hrfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nhrfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);

    return log_syscall("pread", pread(fi->fh, buf, size, offset), 0);
}

/** Escribir datos a un archivo 
 *
 * Escribir debería de retornar exactamente el número de bytes pedidos excepto en error.  
 *
 *  
 */
int hrfs_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nhrfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi
	    );
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);

    return log_syscall("pwrite", pwrite(fi->fh, buf, size, offset), 0);
}

/** OBtener eestadísticas dek file system 
 *
 */
int hrfs_statfs(const char *path, struct statvfs *statv)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nhrfs_statfs(path=\"%s\", statv=0x%08x)\n",
	    path, statv);
    hrfs_fullpath(fpath, path);
    
    // get stats for underlying filesystem
    retstat = log_syscall("statvfs", statvfs(fpath, statv), 0);
    
    log_statvfs(statv);
    
    return retstat;
}

 

/** Release un archivo abierto 
 *
 * Es llamado cuando no hay más derefencias a un archivo abierto: 
 * todos los descriptores de archivo cerrados y los mapas de memoria sin mapear 
 *
 */
int hrfs_release(const char *path, struct fuse_file_info *fi)
{
    log_msg("\nhrfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    log_fi(fi);

    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    return log_syscall("close", close(fi->fh), 0);
}

/** Sincronizar contenidos de un archivo 
 *
 */
int hrfs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    log_msg("\nhrfs_fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	    path, datasync, fi);
    log_fi(fi);
    
    // some unix-like systems (notably freebsd) don't have a datasync call
#ifdef HAVE_FDATASYNC
    if (datasync)
	return log_syscall("fdatasync", fdatasync(fi->fh), 0);
    else
#endif	
	return log_syscall("fsync", fsync(fi->fh), 0);
}

#ifdef HAVE_SYS_XATTR_H
/** Note that my implementations of the various xattr functions use
    the 'l-' versions of the functions (eg hrfs_setxattr() calls
    lsetxattr() not setxattr(), etc).  This is because it appears any
    symbolic links are resolved before the actual call takes place, so
    I only need to use the system-provided calls that don't follow
    them */

/** Modifcar atributos extendidos*/
int hrfs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    char fpath[PATH_MAX];
    
    log_msg("\nhrfs_setxattr(path=\"%s\", name=\"%s\", value=\"%s\", size=%d, flags=0x%08x)\n",
	    path, name, value, size, flags);
    hrfs_fullpath(fpath, path);

    return log_syscall("lsetxattr", lsetxattr(fpath, name, value, size, flags), 0);
}

/** Obtener atributos extendidos */
int hrfs_getxattr(const char *path, const char *name, char *value, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nhrfs_getxattr(path = \"%s\", name = \"%s\", value = 0x%08x, size = %d)\n",
	    path, name, value, size);
    hrfs_fullpath(fpath, path);

    retstat = log_syscall("lgetxattr", lgetxattr(fpath, name, value, size), 0);
    if (retstat >= 0)
	log_msg("    value = \"%s\"\n", value);
    
    return retstat;
}

/** Listar atributos extendidos*/
int hrfs_listxattr(const char *path, char *list, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    char *ptr;
    
    log_msg("\nhrfs_listxattr(path=\"%s\", list=0x%08x, size=%d)\n",
	    path, list, size
	    );
    hrfs_fullpath(fpath, path);

    retstat = log_syscall("llistxattr", llistxattr(fpath, list, size), 0);
    if (retstat >= 0) {
	log_msg("    returned attributes (length %d):\n", retstat);
	if (list != NULL)
	    for (ptr = list; ptr < list + retstat; ptr += strlen(ptr)+1)
		log_msg("    \"%s\"\n", ptr);
	else
	    log_msg("    (null)\n");
    }
    
    return retstat;
}

/** Eliminar atributos extendidos */
int hrfs_removexattr(const char *path, const char *name)
{
    char fpath[PATH_MAX];
    
    log_msg("\nhrfs_removexattr(path=\"%s\", name=\"%s\")\n",
	    path, name);
    hrfs_fullpath(fpath, path);

    return log_syscall("lremovexattr", lremovexattr(fpath, name), 0);
}
#endif

/** Abrir directorio 
 *
 */
int hrfs_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nhrfs_opendir(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    hrfs_fullpath(fpath, path);

    // since opendir returns a pointer, takes some custom handling of
    // return status.
    dp = opendir(fpath);
    log_msg("    opendir returned 0x%p\n", dp);
    if (dp == NULL)
	retstat = log_error("hrfs_opendir opendir");
    
    fi->fh = (intptr_t) dp;
    
    log_fi(fi);
    
    return retstat;
}

/** Leer directorio 
 *
 */

int hrfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    int retstat = 0;
    DIR *dp;
    struct dirent *de;
    
    log_msg("\nhrfs_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
	    path, buf, filler, offset, fi);
    dp = (DIR *) (uintptr_t) fi->fh;
    de = readdir(dp); // toma el error si el primera llamada readdir es null 
    log_msg("    readdir returned 0x%p\n", de);
    if (de == 0) {
	retstat = log_error("hrfs_readdir readdir");
	return retstat;
    }
    do {
	log_msg("calling filler with name %s\n", de->d_name);
	if (filler(buf, de->d_name, NULL, 0) != 0) {
	    log_msg("    ERROR hrfs_readdir filler:  buffer full");
	    return -ENOMEM;
	}
    } while ((de = readdir(dp)) != NULL);
    
    log_fi(fi);
    
    return retstat;
}

/** Release directorio 
 *
 */
int hrfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nhrfs_releasedir(path=\"%s\", fi=0x%08x)\n",
	    path, fi);
    log_fi(fi);
    
    closedir((DIR *) (uintptr_t) fi->fh);
    
    return retstat;
}

/** Sincronizar directorios 
 *
 * 
 */

int hrfs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nhrfs_fsyncdir(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	    path, datasync, fi);
    log_fi(fi);
    
    return retstat;
}

/**
 * Initialize filesystem
 *
 * El valor de retorno se pasa al private_data del fuse_ context 
 * a todas las operaciones de archivo y como parámetro al método destroy 
 *
 */
void *hrfs_init(struct fuse_conn_info *conn)
{
    log_msg("\nhrfs_init()\n");
    
    log_conn(conn);
    log_fuse_context(fuse_get_context());
    
    return HRFS_DATA;
}

/**
 * Limpiar file system 
 *
 * llamado en  filesystem exit.
 */
void hrfs_destroy(void *userdata)
{
    log_msg("\nhrfs_destroy(userdata=0x%08x)\n", userdata);
}

/**
 * Chequear permisos de acceso a archivos Check 
 */
int hrfs_access(const char *path, int mask)
{
    int retstat = 0;
    char fpath[PATH_MAX];
   
    log_msg("\nhrfs_access(path=\"%s\", mask=0%o)\n",
	    path, mask);
    hrfs_fullpath(fpath, path);
    
    retstat = access(fpath, mask);
    
    if (retstat < 0)
	retstat = log_error("hrfs_access access");
    
    return retstat;
}

/**
 * Crear y abrir archivo
 *
 * Si el archivo no existe, crearlo y luego abrirlo 
 */
int hrfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nhrfs_ftruncate(path=\"%s\", offset=%lld, fi=0x%08x)\n",
	    path, offset, fi);
    log_fi(fi);
    
    retstat = ftruncate(fi->fh, offset);
    if (retstat < 0)
	retstat = log_error("hrfs_ftruncate ftruncate");
    
    return retstat;
}

/**
 * Obtener atributos de un archivo 
 */
int hrfs_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nhrfs_fgetattr(path=\"%s\", statbuf=0x%08x, fi=0x%08x)\n",
	    path, statbuf, fi);
    log_fi(fi);
    if (!strcmp(path, "/"))
	return hrfs_getattr(path, statbuf);
    
    retstat = fstat(fi->fh, statbuf);
    if (retstat < 0)
	retstat = log_error("hrfs_fgetattr fstat");
    
    log_stat(statbuf);
    
    return retstat;
}

struct fuse_operations hrfs_oper = {
  .getattr = hrfs_getattr,
  .getdir = NULL,
  .mkdir = hrfs_mkdir,
  .unlink = hrfs_unlink,
  .rmdir = hrfs_rmdir,
  .rename = hrfs_rename,
  .open = hrfs_open,
  .read = hrfs_read,
  .write = hrfs_write,
  .statfs = hrfs_statfs,
  .release = hrfs_release,
  .fsync = hrfs_fsync,
  
#ifdef HAVE_SYS_XATTR_H
  .setxattr = hrfs_setxattr,
  .getxattr = hrfs_getxattr,
  .listxattr = hrfs_listxattr,
  .removexattr = hrfs_removexattr,
#endif
  
  .opendir = hrfs_opendir,
  .readdir = hrfs_readdir,
  .releasedir = hrfs_releasedir,
  .fsyncdir = hrfs_fsyncdir,
  .init = hrfs_init,
  .destroy = hrfs_destroy,
  .access = hrfs_access,
  .ftruncate = hrfs_ftruncate,
  .fgetattr = hrfs_fgetattr
};

void hrfs_usage()
{
    fprintf(stderr, "uso:  bbfs [FUSE opciones de mount] directRoot mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct hrfs_state *hrfs_data;
    if ((getuid() == 0) || (geteuid() == 0)) {
    	fprintf(stderr, "Llamar a HRFS como root podría abrir huecos de seguridad\n");
    	return 1;
    }

    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	hrfs_usage();

    hrfs_data = malloc(sizeof(struct hrfs_state));
    if (hrfs_data == NULL) {
	perror("main calloc");
	abort();
    }

    // Pull the rootdir out of the argument list and save it in my
    // internal data
    hrfs_data->rootdir = realpath(argv[argc-2], NULL);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    hrfs_data->logfile = log_open();
    
    // turn over control to fuse
    fprintf(stderr, "llamando a fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &hrfs_oper, hrfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
