#define  _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define SIZE_BUFFER 50

/*
 * Struct for get_buff functions.
 * Those functions need to return
 * both the data and length
 */
struct string{
  char* line;
  ssize_t len;
};

/* Buffers for producer-consumer pairs */
// Buffer for Input Thread - Line Separator Thread
char* buffer_1[SIZE_BUFFER];
ssize_t line_len_1[SIZE_BUFFER];
// Buffer for Line Separator Thread - Plus Sign Thread
char* buffer_2[SIZE_BUFFER];
ssize_t line_len_2[SIZE_BUFFER];
// Buffer for Plus Sign Thread - Output Thread
char* buffer_3[SIZE_BUFFER];
ssize_t line_len_3[SIZE_BUFFER];

/* Count of items in buffer */
int count_1 = 0;
int count_2 = 0;
int count_3 = 0;

/* Indices for producers */
int producer_index_1 = 0;
int producer_index_2 = 0;
int producer_index_3 = 0;

/* Indices for consumers */
int consumer_index_1 = 0;
int consumer_index_2 = 0;
int consumer_index_3 = 0;

/* Mutexes and Conditions for buffers */
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_3 = PTHREAD_COND_INITIALIZER;

/* Stop Flag and Length of Flag */
static char* STOP_FLAG = "STOP\n";
static ssize_t STOP_LEN = 5;

/* 
 * Function to getline and handle errors
 * returns 0 if successful and -1 for feof or STOP\n
 */
ssize_t
get_user_input(char** input) {
  size_t n = 0;
  ssize_t len = getline(input, &n, stdin);
  if (len == -1) {
    if (feof(stdin)) return -1;
    else err(1, "stdin");
  }
  if (strcmp(*input, STOP_FLAG) == 0) return -1;
  return len;
}

/*
 * Put the line into buffer 1
 */
void
put_buff_1(char* line, ssize_t len) {
  pthread_mutex_lock(&mutex_1);
  buffer_1[producer_index_1] = line;
  line_len_1[producer_index_1] = len;
  count_1++;
  pthread_cond_signal(&full_1);
  pthread_mutex_unlock(&mutex_1);
  producer_index_1++;
}

/*
 * Function that input thread will use to getline
 * and put the line into buffer
 */
void*
get_input(void* args) {
  char* line = NULL;
  ssize_t len = 0;
  while (len != -1) {
    // essentially getline, but with STOP_FLAG
    len = get_user_input(&line);
    put_buff_1(line, len);
  }
  // if get_user_input is -1, put stop flag
  put_buff_1(STOP_FLAG, STOP_LEN);
  return NULL;
}

/*
 * Get line from buffer 1
 */
struct string*
get_buff_1() {
  pthread_mutex_lock(&mutex_1);
  // Wait for buffer to fill
  while (count_1 == 0) {
    pthread_cond_wait(&full_1, &mutex_1);
  }
  char* line = buffer_1[consumer_index_1];
  ssize_t len = line_len_1[consumer_index_1];
  count_1--;
  pthread_mutex_unlock(&mutex_1);
  consumer_index_1++;
  struct string* output = calloc(1, sizeof(struct string));
  output->line = strdup(line);
  output->len = len;
  return output;
}

/*
 * Put line without line separators into buffer 2
 */
void
put_buff_2(char* line, ssize_t len) {
  pthread_mutex_lock(&mutex_2);
  buffer_2[producer_index_2] = line;
  line_len_2[producer_index_2] = len;
  count_2++;
  pthread_cond_signal(&full_2);
  pthread_mutex_unlock(&mutex_2);
  producer_index_2++;
}

/*
 * Function that line separator thread will use
 * to remove line separators and assign that line
 * to buffer 2 for plus sign thread
 */
void*
remove_line_sep(void* args) {
  char* line = "";
  ssize_t len;
  while (strcmp(line, STOP_FLAG) != 0) {
    struct string* string = get_buff_1();
    line = strdup(string->line);
    len = string->len;
    /* 
     * Need to break early once Stop Flag is
     * met since the next lines remove the
     * line separator
     */
    if (strcmp(line, STOP_FLAG) == 0) {
      put_buff_2(STOP_FLAG, STOP_LEN);
      return NULL;
    }
    if (line[len - 1] == '\n') {
      line[len - 1] = ' ';
    }
    put_buff_2(line, len);
  }
  return NULL;
}

/*
 * Get line from buffer 2
 */
struct string*
get_buff_2() {
  pthread_mutex_lock(&mutex_2);
  // when buffer is empty, it needs to wait
  while (count_2 == 0) {
    pthread_cond_wait(&full_2, &mutex_2);
  }
  char* line = buffer_2[consumer_index_2];
  ssize_t len = line_len_2[consumer_index_2];
  count_2--;
  pthread_mutex_unlock(&mutex_2);
  consumer_index_2++;
  struct string* output = calloc(1, sizeof(struct string));
  output->line = line;
  output->len = len;
  return output;
}

/*
 * Put line that has "++" replaced to '^'
 * into buffer 3
 */
void
put_buff_3(char* line, ssize_t len) {
  pthread_mutex_lock(&mutex_3);
  buffer_3[producer_index_3] = line;
  line_len_3[producer_index_3] = len;
  count_3++;
  pthread_cond_signal(&full_3);
  pthread_mutex_unlock(&mutex_3);
  producer_index_3++; 
}

/*
 * Function that plus sign thread will run
 * to remove "++" to '^'. Takes lines from buffer 2,
 * finds any "++" and replaces it with '^' by  
 * creating a new string and adding non-"++" sequences
 * and adding '^' when "++" sequence is found
 */
void*
remove_plus_signs(void* args) {
  char* line = "";
  ssize_t len = 0;
  while (strcmp(line, STOP_FLAG) != 0) {
    struct string* string = get_buff_2();
    line = string->line;
    len = string->len;
    char* new_line = malloc(len * sizeof(char));
    ssize_t new_line_len = 0;
    for (ssize_t i = 0; i < len; ++i) {
      if (line[i] == '+' && line[i + 1] == '+') {
        new_line[new_line_len++] = '^';
        i++;
      }
      else {
        new_line[new_line_len++] = line[i];
      }
    }
    new_line = realloc(new_line, new_line_len);
    put_buff_3(new_line, new_line_len);
  }
  // The last line given is the Stop Flag
  return NULL;
}

/*
 * Get line from buffer 3
 */
struct string*
get_buff_3() {
  pthread_mutex_lock(&mutex_3);
  // When buffer is empty, wait
  while (count_3 == 0) {
    pthread_cond_wait(&full_3, &mutex_3);
  }
  char* line = buffer_3[consumer_index_3];
  ssize_t len = line_len_3[consumer_index_3];
  count_3--;
  pthread_mutex_unlock(&mutex_3);
  consumer_index_3++;
  // Create string struct to return both string data and length
  struct string* output = calloc(1, sizeof(struct string));
  output->line = line;
  output->len = len;
  return output;
}

/*
 * Function output thread will use to print the input
 * only when 80 characters have been met. It will print a newline
 * after each 80 character line. Any extra characters that do
 * not fill 80 characters will not be printed. If STOP_FLAG
 * is encountered, stop printing and exit immediately.
 */
void*
write_line(void* args) {
  struct string* string = get_buff_3();
  char* line = string->line;
  ssize_t len = string->len;
  char output[81];
  // every 80 char needs \n to end it
  output[80] = '\n';
  int out_index = 0;
  while (strcmp(line, STOP_FLAG) != 0) {
    // Keeps track of how much of line has been read
    int i = 0;
    
    add_more:;

    // Need to reuse i if we jump back to add_more:
    while (i < len && out_index < 80) {
      output[out_index] = line[i];
      out_index++;
      i++;
    }
    // Only print 80 characters from the buffer and add \n
    if (out_index == 80) {
      fflush(stdout);
      write(1, output, 81);
      out_index = 0;
      /* 
       * since the line from the buffer has not been used
       * up yet, go back and fill output more
       */
      if (i < len) goto add_more;
    }
    string = get_buff_3();
    line = string->line;
    len = string->len;
  }
  return NULL;
}

int main(void)
{
  pthread_t input_tid, line_sep_tid, plus_sign_tid, output_tid;
  pthread_create(&input_tid, NULL, get_input, NULL);
  pthread_create(&line_sep_tid, NULL, remove_line_sep, NULL);
  pthread_create(&plus_sign_tid, NULL, remove_plus_signs, NULL);
  pthread_create(&output_tid, NULL, write_line, NULL);

  pthread_join(input_tid, NULL);
  pthread_join(line_sep_tid, NULL);
  pthread_join(plus_sign_tid, NULL); 
  pthread_join(output_tid, NULL);
  return EXIT_SUCCESS;
}
