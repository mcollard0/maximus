/* Socket-pair regressions for the actual driver, including its private parser. */
#include <assert.h>
#include "ipcomm.c"

void logit(char *format, ...) { (void)format; }
void _fast NoMem(void) { abort(); }

static void init_handle(HCOMM hc, int fd[2])
{
  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);
  memset(hc, 0, sizeof(*hc));
  hc->h = fd[0];
  hc->fDCD = TRUE;
  hc->peekHack = -1;
}

/* A screen can ask for input while burst mode is still active. Its prompt
 * must reach the peer without the peer first having to send a key. */
static void prompt_flushes(void)
{
  int operation;
  for (operation = 0; operation < 5; ++operation)
  {
    struct _hcomm hc;
    int fd[2];
    unsigned char tx[128], received[128];
    char prompt[] = "Welcome! Press Enter";
    ssize_t n;
    init_handle(&hc, fd);
    hc.txCoalesce = tx;
    hc.txCoalesceCap = sizeof(tx);
    ComBurstMode(&hc, TRUE);
    assert(ComWrite(&hc, prompt, sizeof(prompt) - 1));
    assert(recv(fd[1], received, sizeof(received), MSG_DONTWAIT) == -1);
    assert(errno == EAGAIN || errno == EWOULDBLOCK);
    switch (operation)
    {
      case 0: assert(ComPeek(&hc) == -1); break;
      case 1: assert(!ComRxWait(&hc, 0)); break;
      case 2: assert(ComTxWait(&hc, 0)); break;
      case 3: assert(ComOutCount(&hc) == 0); break;
      case 4:
        hc.peekHack = 'X';
        assert(ComPeek(&hc) == 'X');
        assert(hc.peekHack == 'X');
        break;
    }
    n = recv(fd[1], received, sizeof(received), MSG_DONTWAIT);
    assert(n == sizeof(prompt) - 1);
    assert(memcmp(received, prompt, n) == 0);
    close(fd[0]);
    close(fd[1]);
  }
}

/* Test every possible partition of a short wire sequence, including one-byte
 * reads and a single bulk read. Allocate exactly the requested read size so
 * ASan detects accesses beyond that buffer. */
static void partitions(const unsigned char *wire, size_t len,
                       const unsigned char *expected, size_t expected_len,
                       int binary)
{
  unsigned mask;
  for (mask = 0; mask < (1u << (len - 1)); ++mask)
  {
    struct _hcomm hc;
    int fd[2];
    size_t start = 0, end, used = 0;
    unsigned char result[64];
    init_handle(&hc, fd);
    hc.telnetOptions = binary ? mopt_TRANSMIT_BINARY : 0;
    for (end = 1; end <= len; ++end)
    {
      if (end == len || (mask & (1u << (end - 1))))
      {
        size_t count = end - start;
        unsigned char *buf = malloc(count);
        ssize_t n;
        assert(buf);
        assert(write(fd[1], wire + start, count) == (ssize_t)count);
        n = telnet_read(&hc, buf, count);
        assert(n >= 0 && n <= (ssize_t)count);
        memcpy(result + used, buf, n);
        used += n;
        free(buf);
        start = end;
      }
    }
    assert(used == expected_len);
    assert(memcmp(result, expected, used) == 0);
    close(fd[0]);
    close(fd[1]);
  }
}

int main(void)
{
  struct _hcomm hc;
  int fd[2], i;
  unsigned char buf[32];
  const unsigned char commands[] = {
    255, 254, 36, 255, 253, 3, 255, 251, 1,
    255, 251, 3, 255, 254, 31, 255, 253, 0
  };
  /* Fail boundedly if a malformed command causes a blocking read or loop. */
  alarm(10);
  prompt_flushes();
  partitions((const unsigned char *)"AB\r\0", 4,
             (const unsigned char *)"AB\r", 3, 0);
  partitions((const unsigned char *)"ABC\r", 4,
             (const unsigned char *)"ABC\r", 4, 0);
  partitions((const unsigned char *)"\rX", 2,
             (const unsigned char *)"\rX", 2, 0);
  partitions((const unsigned char *)"A\r\nB\r\0C\0", 8,
             (const unsigned char *)"A\rB\rC", 5, 0);
  partitions((const unsigned char *)"\xff\xfa\x18\x01\xff\xff\xff\xf0X", 9,
             (const unsigned char *)"X", 1, 0);
  partitions((const unsigned char *)"\xff\xfb\x03\xff\xfd\x01X", 7,
             (const unsigned char *)"X", 1, 0);
  partitions((const unsigned char *)"\r\0\n\xff\xffX", 6,
             (const unsigned char *)"\r\0\n\xffX", 5, 1);
  partitions((const unsigned char *)"\xff\xfb\0\0\xff\xff", 6,
             (const unsigned char *)"\0\xff", 2, 0);
  partitions((const unsigned char *)"\xff\xfc\0\r\nX", 6,
             (const unsigned char *)"\rX", 2, 1);

  init_handle(&hc, fd);
  hc.telnetOptions = mopt_TRANSMIT_BINARY | mopt_ECHO | mopt_SGA;
  setTelnetOption(&hc, cmd_WONT, opt_TRANSMIT_BINARY);
  assert(hc.telnetOptions == (mopt_ECHO | mopt_SGA));
  hc.telnetPendingOptions = mopt_ECHO | mopt_SGA;
  setTelnetOption(&hc, cmd_DONT, opt_ECHO);
  assert(hc.telnetPendingOptions == mopt_SGA);
  negotiateTelnetOptions(&hc);
  assert(read(fd[1], buf, sizeof(buf)) == sizeof(commands));
  assert(memcmp(buf, commands, sizeof(commands)) == 0);

  /* Carrier checks must leave negotiation bytes for the parser. */
  assert(write(fd[1], "\xff\xfb\x03X", 4) == 4);
  for (i = 0; i < 30; ++i)
    assert(ComIsOnline(&hc));
  for (i = 0; i < 3; ++i)
    assert(ComGetc(&hc) == -1);
  assert(ComPeek(&hc) == 'X');
  assert(ComGetc(&hc) == 'X');
  assert(write(fd[1], "\r", 1) == 1);
  assert(ComPeek(&hc) == '\r');
  assert(ComGetc(&hc) == '\r');
  assert(write(fd[1], "\nX", 2) == 2);
  assert(ComGetc(&hc) == -1);
  assert(ComGetc(&hc) == 'X');
  close(fd[0]);
  close(fd[1]);

  init_handle(&hc, fd);
  assert(write(fd[1], "\xff\xfa\x18\x01", 4) == 4);
  close(fd[1]);
  assert(telnet_read(&hc, buf, 4) == 0);
  assert(telnet_read(&hc, buf, 4) == 0);
  close(fd[0]);
  puts("ipcomm socket-pair regressions passed");
  return 0;
}
