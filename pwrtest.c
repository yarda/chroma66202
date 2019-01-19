/*
  Frontend for measuring energy consumption with Chroma 66202
  power meter through USBTMC.
  Copyright (C) 2013 Yarda <jskarvad@redhat.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the.
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define DEV "/dev/usbtmc0"
#define IDN "*IDN?\n"
#define MEAS "MEAS?\n"

#define BUFLEN 1024
#define BUFSIZE 1024
#define DEFAULT_WINDOW "1.0"
#define DEFAULT_REPEAT 1
#define DEFAULT_DELAY 10

/* Let the device some time to process the command (in usec) */
#define DELAY 10000

#define cr(arg) \
(command(f, (arg)) && response(f, buf, BUFLEN))

#define c(arg) \
command(f, (arg))

#define cp(arg1, arg2) \
(strncpy(buf, (arg1), BUFLEN - 1), \
  strncat(buf, (arg2), BUFLEN - 1 - strlen(buf)), \
  strncat(buf, "\n", BUFLEN - 1 - strlen(buf)), \
command(f, buf))

int f;
int compact;
int dummy;
int repeat = DEFAULT_REPEAT;
int delay = DEFAULT_DELAY;
int tintegratei = 0;
volatile int ssigusr1 = 0;
volatile int ssigterm = 0;
unsigned long long iters;
time_t tt, tts;
double u = 0.0f;
double i = 0.0f;
double frq = 0.0f;
double p = 0.0f;
double pf = 0.0f;
char shunt[5] = "AUTO";
char irange[5] = "A8";
char urange[5] = "AUTO";
char window[6] = DEFAULT_WINDOW;
char tintegrate[] = "1";
char buf[BUFLEN] = {0};
char timebuf[BUFSIZE] = {0};
struct sigaction acts_sigusr1 = {{0}};
struct sigaction acts_sigusr2 = {{0}};
struct sigaction acts_term = {{0}};
struct sigaction act = {{0}};

void sig_handler(int sig)
{
  switch (sig)
  {
    case SIGUSR1:
      ssigusr1 = 1;
      break;
    case SIGUSR2:
    case SIGTERM:
      ssigterm = 1;
  }
}

void cleartrail(char *buf)
{
  int x;

  x = strlen(buf);
  while (x-- && (buf[x] == '\r' || buf[x] == '\n'))
    buf[x] = 0;
}

int command(int f, char *command)
{
  ssize_t x;
#ifdef DEBUG
  fprintf(stderr, "%s\n", command);
#endif
  x = write(f, command, strlen(command)) != -1;
  usleep(DELAY);
  return (int) x;
}

int response(int f, char *buffer, int buflen)
{
  memset(buffer, 0, buflen);
  return read(f, buffer, buflen - 1) != -1;
}

void integr(void)
{
  double g;

  cleartrail(buf);
  sscanf(strtok(buf, ";"), "%lf", &g);
  if (g < 0.0f)
    u = g;
  if (u >= 0.0f)
    u += (g - u) / iters;
  sscanf(strtok(NULL, ";"), "%lf", &g);
  if (g < 0.0f)
    i = g;
  if (i >= 0.0f)
    i += (g - i) / iters;
  sscanf(strtok(NULL, ";"), "%lf", &g);
  if (g < 0.0f)
    frq = g;
  if (frq >= 0.0f)
    frq += (g - frq) / iters;
  sscanf(strtok(NULL, ";"), "%lf", &g);
  if (g < 0.0f)
    p = g;
  if (p >= 0.0f)
    p += (g - p) / iters;
  sscanf(strtok(NULL, ";"), "%lf", &g);
  if (g < 0.0f)
    pf = g;
  if (pf >= 0.0f)
    pf += (g - pf) / iters;
}

void printresults(void)
{
  memset(timebuf, 0, BUFSIZE);
  time(&tt);
  if (compact)
  {
    printf("%lu;%f;%g\n", tt - tts, p, p * (tt - tts));
//    fflush(stdout);
  }
  else
  {
    strftime(timebuf, BUFSIZE, "%Y-%m-%d %H:%M:%S %Z", localtime(&tt));
    printf("%s; %f; %f; %f; %f; %f\n", timebuf, u, i, frq, p, pf);
//    fflush(stdout);
  }
}

int main(int argc, char *argv[])
{
  int ret;
  struct timespec rem = {0};

  // line buffering stdout
  //setvbuf(stdout, NULL, _IOLBF, 0);
  setlinebuf(stdout);
  compact = 0;
  time(&tts);
  if (argc < 1 || argc > 2)
  {
    printf("usage: %s [-e]\n", argv[0]);
    printf("  -c  compact mode, show only the time, power and energy");
    return 0;
  }

  if (argc == 2 && strcmp(argv[1], "-c") == 0)
  {
    compact = 1;
  }

  if ((f = open(DEV, O_RDWR)) < 0)
  {
    fprintf(stderr, "Error opening device: %s\n", DEV);
    return 1;
  }
  memset(buf, 0, BUFLEN);
  u = 0.0f;
  i = 0.0f;
  frq = 0.0f;
  p = 0.0f;
  pf = 0.0f;
  iters = 1;
  ssigusr1 = 0;
  ssigterm = 0;
  act.sa_handler = sig_handler;
  sigaction(SIGUSR1, &act, NULL);
  sigaction(SIGUSR2, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
  if (!compact)
  {
    printf("Time; U; I; P; PF\n");
//    fflush(stdout);
  }
/*
  if (read(f, buf, BUFLEN - 1) < 0)
  {
    fprintf(stderr, "Error reading device.\n");
  }
  else
  {
    printf("Buffer: %s\n", buf);
  }
*/
/*  if (!c("SYST:HEAD OFF\n") ||
      !c("SYST:TRAN:SEP 1\n") || !c("SYST:TRAN:TERM 0\n") ||
      !c("SOUR:POW:INT 0\n") ||
      !cp("SOUR:CURR:SHUN ", shunt) || !c("SOUR:MEAS:MODE WINDOW\n") ||
      !cp("SOUR:MEAS:WIND ", window) || !c("SOUR:TRIG:MODE NONE\n") ||
      !cp("SOUR:CURR:RANG ", irange) || !cp("SOUR:VOLT:RANG ", urange) ||
      !cp("SOUR:POW:INT ", tintegrate) || !cr("*IDN?"))
  {
    fprintf(stderr, "Error reading device.\n");
  }
  else
  {
    printf("Buffer: %s\n", buf);
  }
*/
  if (!c("SYST:HEAD OFF\n") ||
      !c("SYST:TRAN:SEP 1\n") || !c("SYST:TRAN:TERM 0\n") ||
      !c("SOUR:POW:INT 0\n") ||
      !cp("SOUR:CURR:SHUN ", shunt) || !c("SOUR:MEAS:MODE WINDOW\n") ||
      !cp("SOUR:MEAS:WIND ", window) || !c("SOUR:TRIG:MODE NONE\n") ||
      !cp("SOUR:CURR:RANG ", irange) || !cp("SOUR:VOLT:RANG ", urange) ||
      !cp("SOUR:POW:INT ", tintegrate) || !cr("MEAS? V,I,FREQ,W,PF\n"))
  {
    fprintf(stderr, "Error reading device.\n");
  }
  sleep(1);
  while (buf[0] == '-' && buf[1] == '1')
  {
    sleep(1);
    dummy = cr("FETC? V,I,FREQ,W,PF\n");
  }
  integr();
  while (!ssigterm)
  {
    iters++;
    if (iters)
    {
      dummy = cr("FETC? V,I,FREQ,W,PF\n");
      integr();
    }
    else
    {
      fprintf(stderr, "Error: overflow.\n");
      ssigterm = 1;
    }
    rem.tv_sec = 1;
    rem.tv_nsec = 0;
    do
    {
      ret = nanosleep(&rem, &rem);
      if (ret < 0 && errno != EINTR)
         fprintf(stderr, "error %d returned by nanosleep", ret);

      if (ssigusr1)
      {
        ssigusr1 = 0;
        printresults();
      }
    }
    while (ret < 0 && errno == EINTR);
  }
  printresults();
  close(f);
  return 0;
}
