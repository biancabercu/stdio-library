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
	long bytesRead;
	int ferrorFlag;
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
	f->ferrorFlag = 0;
	f->bytesRead = 0;
	f->lastOperat = 0;
	return f;
}

/* close the stream - the SO_FILE struct */
int so_fclose(SO_FILE *stream)
{
	int resultofFlush = so_fflush(stream);
	int resultofClose = close(stream->fd);

	/*free stream despite the flush/close result */
	free(stream->buffer);
	free(stream);

	if (resultofClose != -1 && resultofFlush != EOF)
		return resultofClose;
	return EOF;
}

/*returns the file descriptor's struct number*/
int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}
/*flush the stream buffer */
int so_fflush(SO_FILE *stream)
{
	if (stream != NULL && stream->cursorPos != 0) {
		if (stream->lastOperat == 1) {
			/*write data to disk if there is any in the buffer*/
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
		stream->ferrorFlag = 1;
		return EOF;
	}
	/*if buffer is emtpy, nothing to do*/
	if (stream->cursorPos == 0)
		return 0;
	/*otherwise there was an error*/
	stream->ferrorFlag = 1;
	return EOF;

}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	/*flush regardles of seek result*/
	so_fflush(stream);

	int seekedPos = lseek(stream->fd, offset, whence);

	if (seekedPos >= 0) {
		stream->bytesRead = seekedPos;
		return 0;
	}
	return -1;
}
/*calculate stream pos =bytes read +pos in buf*/
long so_ftell(SO_FILE *stream)
{
	return stream->bytesRead + stream->cursorPos;
}

/*reads nmemb, each of size size from stream
 * and puts them in ptr
 */
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int bytestoRead = size * nmemb;

	for (int i = 0; i < bytestoRead; i++) {
		int charRead = so_fgetc(stream);

		if (so_feof(stream) == 0)
			*((char *)ptr + i) = charRead;
		else
			return i / size;
	}
	return bytestoRead;
}

/*write data by calling fputc*/
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	char *charPtr = (char *) ptr;
	int bytestoWrite = nmemb * size;

	for (int i = 0; i < bytestoWrite; i++) {
		if (so_fputc(*(charPtr + i), stream) == EOF
				&& *(charPtr + i) != EOF)
			return i / size;
	}
	return bytestoWrite;
}
	/*get a char from stream*/
int so_fgetc(SO_FILE *stream)
{
	stream->lastOperat = 2;
	int readBytes = 0;
	/*read from buffer if you can, otherwise do syscall*/
	/*daca nu se poate citi din buffer, dam read, alfel cotinuam citirea
	 * din buffer
	 */
	if (stream->cursorPos == stream->cursorSizeBuff
		|| stream->cursorPos == 0) {
		readBytes = read(stream->fd, stream->buffer, SIZEBUFFER);
		if (readBytes > 0) {
			if (stream->cursorPos == stream->cursorSizeBuff)
				stream->bytesRead += readBytes;

			stream->cursorSizeBuff = readBytes;
			stream->cursorPos = 0;
		} else {
			stream->ferrorFlag = 1;
			return EOF;
		}
	}
	int response = (int)stream->buffer[stream->cursorPos];

	stream->cursorPos++;

	return response;
}
/* put character in stream */
int so_fputc(int c, SO_FILE *stream)
{
	stream->lastOperat = 1;
	if (stream != NULL) {
		/*flush if buffer is full*/
		if (stream->cursorPos >= stream->cursorSizeBuff) {
			stream->bytesRead += stream->cursorSizeBuff;
			if (so_fflush(stream) == EOF)
				return EOF;
		}
		/*otherwise add to bufffer*/
		stream->buffer[stream->cursorPos] = c;
		stream->cursorPos++;
		return c;
	}
	stream->ferrorFlag = 1;
	return EOF;
}

/*return feof*/
int so_feof(SO_FILE *stream)
{
	return stream->ferrorFlag;
}
/*returns ferror*/
int so_ferror(SO_FILE *stream)
{
	return stream->ferrorFlag;
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
	f->ferrorFlag = 0;
	f->bytesRead = 0;
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
