#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "so_stdio.h"

#define EOF (-1)
#define SIZEBUFFER 4096

struct _so_file {
	int fd;
	char *buffer;
	long read_bytes;
	int flag;
	int lastOperat;
	int pid;
	int cursorPos;
	int cursorSizeBuff;
};


/*opens the file in approp number
 * &init the struct values
 */
SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	int fd = -1;

	if (strcmp(mode, "w") == 0)
		fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (strcmp(mode, "w+") == 0)
		fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (strcmp(mode, "r") == 0)
		fd = open(pathname, O_RDONLY, 0644);
	if (strcmp(mode, "r+") == 0)
		fd = open(pathname, O_RDWR, 0644);
	if (strcmp(mode, "a") == 0)
		fd = open(pathname, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (strcmp(mode, "a+") == 0)
		fd = open(pathname, O_RDWR | O_CREAT | O_APPEND, 0644);
	if (fd < 0)
		return NULL;

	SO_FILE *f = (SO_FILE *)malloc(sizeof(SO_FILE));

	f->buffer = (char *)malloc(sizeof(char) * SIZEBUFFER);

	memset(f->buffer, 0, SIZEBUFFER);

	f->cursorPos = 0;
	f->cursorSizeBuff = SIZEBUFFER;
	f->fd = fd;
	f->flag = 0;
	f->read_bytes = 0;
	f->lastOperat = 0;
	return f;
}

/* close the stream - the SO_FILE struct */
int so_fclose(SO_FILE *stream)
{
	int flush_res = so_fflush(stream);
	int close_res = close(stream->fd);

	free(stream->buffer);
	free(stream);

	if (close_res != -1 && flush_res != EOF)
		return close_res;
	return EOF;
}

/*fileno = descriptor-ul , file pentru stream*/
int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}
/*functie de flush +write on disk */
int so_fflush(SO_FILE *stream)
{
	if (stream != NULL && stream->cursorPos != 0) {
		if (stream->lastOperat == 1) {
			/*in cazul in care raman date nescrise, se vor scrie*/
			if (write(stream->fd, stream->buffer,
						stream->cursorPos) != -1) {
				memset(stream->buffer, 0, SIZEBUFFER);
				stream->cursorPos = 0;
				stream->cursorSizeBuff = SIZEBUFFER;

				return 0;
			}
		}
		/* 0 buffer after read */
		if (stream->lastOperat == 2) {
			memset(stream->buffer, 0, SIZEBUFFER);
			stream->cursorPos = 0;
			stream->cursorSizeBuff = SIZEBUFFER;

			return 0;
		}
		stream->flag = 1;
		return EOF;
	}
	/*if buffer is emtpy, nothing to do*/
	if (stream->cursorPos == 0)
		return 0;
	/*otherwise there was an error*/
	stream->flag = 1;
	return EOF;

}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	/*flush regardles of seek result*/
	so_fflush(stream);

	int seekedPos = lseek(stream->fd, offset, whence);

	if (seekedPos >= 0) {
		stream->read_bytes = seekedPos;
		return 0;
	}
	return -1;
}
/*calculate stream pos =bytes read +pos in buf*/
long so_ftell(SO_FILE *stream)
{
	return stream->read_bytes + stream->cursorPos;
}

/*reads nmemb, each of size size from stream
 * and puts them in ptr
 */
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int toRead_bytes = size * nmemb;
	int i = 0;

	while (i < toRead_bytes) {
		int charRead = so_fgetc(stream);

		if (so_feof(stream) == 0)
			*((char *)ptr + i) = charRead;
		else
			return i / size;
		i++;
	}
	return toRead_bytes;
}

/*write data by calling fputc*/
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	char *charPtr = (char *) ptr;
	int toWrite_bytes = nmemb * size;
	int i = 0;

	while (i < toWrite_bytes) {
		if (so_fputc(*(charPtr + i), stream) == EOF
				&& *(charPtr + i) != EOF)
			return i / size;
		i++;
	}
	return toWrite_bytes;
}
/*se citeste un caracter din stream, fd*/
int so_fgetc(SO_FILE *stream)
{
	stream->lastOperat = 2;
	int readBytes = 0;
	/*daca nu se poate citi din buffer, dam read, alfel cotinuam citirea
	 * din buffer
	 */
	/*citeste din buffer daca se poate, altfel va folosi apelul de sistem
	 * primul if va face verificarea buffer-ului,
	*/
	if (stream->cursorPos == stream->cursorSizeBuff
		|| stream->cursorPos == 0) {
		readBytes = read(stream->fd, stream->buffer, SIZEBUFFER);
		if (readBytes > 0) {
			if (stream->cursorPos == stream->cursorSizeBuff)
				stream->read_bytes += readBytes;

			stream->cursorSizeBuff = readBytes;
			stream->cursorPos = 0;
		} else {
			stream->flag = 1;
			return EOF;
		}
	}
	int character = (int)stream->buffer[stream->cursorPos];

	stream->cursorPos++;

	return character;
}
/* pune caracterul in stream, folosind flush +read on disk*/
int so_fputc(int c, SO_FILE *stream)
{
	stream->lastOperat = 1;
	if (stream != NULL) {
		/*se verifica daca buffer-ul e deja plin*/
		if (stream->cursorPos >= stream->cursorSizeBuff) {
			stream->read_bytes += stream->cursorSizeBuff;
			if (so_fflush(stream) == EOF)
				return EOF;
		}
		/*se adauga caracterul la buffer-ul din stream */
		/* si se creste index-ul catre urmatorul spatiu */
		stream->buffer[stream->cursorPos] = c;
		stream->cursorPos++;
		return c;
	}
	stream->flag = 1;
	return EOF;
}

/*folosesc un flag de eroare*/
int so_feof(SO_FILE *stream)
{
	return stream->flag;
}
/*folosesc un flag de eroare*/
int so_ferror(SO_FILE *stream)
{
	return stream->flag;
}

SO_FILE *so_popen(const char *command, const char *type)
{
	int cpid = -1;
	int fds[2];

	pipe(fds);

	SO_FILE *f = (SO_FILE *)malloc(sizeof(SO_FILE));

	f->buffer = (char *)malloc(sizeof(char) * SIZEBUFFER);
	cpid = fork();
	switch (cpid) {
	case -1:
		/*aici apar erorile la forking*/
		free(f->buffer);
		free(f);
		return NULL;
	case 0:
		/*aici pentru child process*/
		if (strcmp(type, "r") == 0) {
			close(fds[0]);
			if (fds[1] != STDOUT_FILENO) {
				dup2(fds[1], STDOUT_FILENO);
				close(fds[1]);
				fds[1] = STDOUT_FILENO;
			}
		} else if (strcmp(type, "w") == 0) {
			close(fds[1]);
			if (fds[0] != STDIN_FILENO) {
				dup2(fds[0], STDIN_FILENO);
				close(fds[0]);
			}
		}
		execlp("/bin/sh", "sh", "-c", command, NULL);
		return NULL;
	default:
		/*pentru parent process*/
		break;
	}

	/*de aici doar parintele proces*/
	int fd;

	if (strcmp(type, "w") == 0) {
		fd = fds[1];
		close(fds[0]);
	}
	if (strcmp(type, "r") == 0) {
		close(fds[1]);
		fd = fds[0];
	}

	memset(f->buffer, 0, SIZEBUFFER);
	f->cursorPos = 0;
	f->cursorSizeBuff = SIZEBUFFER;
	f->flag = 0;
	f->read_bytes = 0;
	f->lastOperat = 0;
	f->pid = cpid;
	f->fd = fd;

	return f;
}

int so_pclose(SO_FILE *stream)
{
	int wait_pid = stream->pid;
	int pid = -1, status = -1;

	so_fclose(stream);

	pid = waitpid(wait_pid, &status, 0);

	if (pid == -1)
		return -1;
	return status;
	return -1;
}
