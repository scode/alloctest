#define _GNU_SOURCE

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static int read_and_discard_line()
{
  char* line = NULL;
  size_t linecap = 0;
  if (getline(&line, &linecap, stdin) == -1) {
    printf("getline() failed\n");
    return -1;
  }
  free(line);
  return 0;
}

int main(int argc, const char* const* argv)
{
  bool use_malloc = false;
  bool use_mmap = false;

  if (argc != 3 ||
      (strcmp(argv[1], "malloc") != 0 &&
       strcmp(argv[1], "mmap") != 0)) {
    fprintf(stderr, "usage: alloc <malloc|mmap> <amount-in-mb>\n");
    fprintf(stderr, "example: ./alloc mmap 100\n");
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "malloc") == 0) {
    use_malloc = true;
  } else {
    use_mmap = true;
  }

  size_t mbcount = atoi(argv[2]); /* nevermind validation */

  printf("allocating %d MB\n", mbcount);
  uint8_t* p;
  if (use_malloc) {
    p = (uint8_t*)malloc(mbcount * 1024ULL * 1024ULL);

    if (p == NULL) {
      fprintf(stderr, "malloc()/mmap() failed: %s", strerror(errno));
      return EXIT_FAILURE;
    }
  } else if (use_mmap) {
    char* template = strdup("./alloc-mmap.XXXXXXXX");
    if (template == NULL) {
      fprintf(stderr, "strdup bork\n");
      return EXIT_FAILURE;
    }

    int fd = mkstemp(template);

    if (fd < 0) {
      fprintf(stderr, "mkstemp() failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
    }

    if (lseek(fd, mbcount * 1024ULL * 1024ULL, SEEK_CUR) == -1) {
      fprintf(stderr, "lseek() failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
    }

    // Make sure we have mbcount MB of valid file to mmap().
    if (write(fd, "trailer", sizeof("trailer")) <= 0) {
      fprintf(stderr, "write failed/short write\n");
      return EXIT_FAILURE;
    }

    p = mmap(NULL, mbcount * 1024ULL * 1024ULL, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
      fprintf(stderr, "mmap() failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
    }
  } else {
    fprintf(stderr, "bork\n");
    return EXIT_FAILURE;
  }

  printf("allocated - press enter to fill");
  if (read_and_discard_line() == -1) {
    return EXIT_FAILURE;
  }

  printf("filling\n");
  for (size_t i = 0; i < mbcount * 1024ULL * 1024ULL; i++) {
    p[i] = 'w';
  }

  printf("done - press enter to exit\n");

  if (read_and_discard_line() == -1) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

