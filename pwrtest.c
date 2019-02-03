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
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define PROGNAME pwrtest
#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define STRING(S) STR_HELPER(S)
#define STR_HELPER(S) #S
#define PROGNAME_STRING STRING(PROGNAME)
#define VERSION_STRING STRING(VERSION_MAJOR) "." STRING(VERSION_MINOR)
#define COPYRIGHT_STRING \
"Copyright (C) 2010-2019 Yarda <jskarvad@redhat.com>"

#define E_SUCCESS   0
#define E_MISSARG   1
#define E_UNKOPT    2
#define E_WRONGARG  3

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

int fcompact = 0;
int ftest = 0;
int flog = 0;
int f;
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
double e = 0.0f;
char shunt[5] = "AUTO";
char irange[5] = "A8";
char urange[5] = "AUTO";
char window[6] = DEFAULT_WINDOW;
char tintegrate[] = "1";
char buf[BUFLEN] = { 0 };
char timebuf[BUFSIZE] = { 0 };
char path_log[PATH_MAX] = { 0 };
// in ms
int averaging_interval = 1000;
struct sigaction act = {{ 0 }};
FILE *file_log;

struct option long_options[] = {
  {"help", 0, 0, 'h'},
  {"version", 0, 0, 'v'},
  {0, 0, 0, 0}
};

void sig_handler(int sig)
{
  switch (sig)
  {
    case SIGUSR1:
      ssigusr1 = 1;
      break;
    case SIGINT:
    case SIGUSR2:
    case SIGTERM:
      ssigterm = 1;
  }
}

int err(int code)
{
  char *str;

  switch(code)
  {
    case E_SUCCESS:
      str = "No error.";
      break;
    case E_MISSARG:
      str = "Missing argument.";
      break;
    case E_UNKOPT:
      str = "Unknown option.";
      break;
    case E_WRONGARG:
      str = "Wrong argument.";
      break;
   default:
      str = "Unknown error.";
  }
  fprintf(stderr, "%s\n", str);
  return code;
}

void version (void)
{
  printf("%s v%s\n", PROGNAME_STRING, VERSION_STRING);
  printf("%s\n", COPYRIGHT_STRING);
  printf("License GPLv3+: GNU GPL version 3 or later ");
  printf("<http://gnu.org/licenses/gpl.html>\n");
  printf("This is free software: ");
  printf("you are free to change and redistribute it.\n");
  printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
}

void help(char *argv[])
{
  printf("Chroma 66202 power meter frontend.\n");
  printf("Version %s %s\n", VERSION_STRING, COPYRIGHT_STRING);
  printf("This program is distributed under the terms of the ");
  printf("GNU General Public License.\n\n");
  printf("Usage: %s [OPTIONS]\n", argv[0]);
  printf("  -lLOG          Log all readings to the LOG.\n");
  printf("  -c             Compact mode, show only the time, power and energy.\n");
  printf("  -t             Test mode, just check presence of the Chroma 66202 power ");
  printf("meter and exits.\n");
  printf("energy.\n");
  printf("  --help, -h     Show this help.\n");
  printf("  --version, -v  Show version of this program.\n\n");
}

int parse_args(int argc, char *argv[])
{
  int opt, option_index;

  while ((opt =
          getopt_long (argc, argv, "tcl:hv", long_options,
                       &option_index)) != -1)
  {
      switch (opt)
      {
        case 'h':
          help(argv);
          return -1;
        case 'v':
          version();
          return -1;
        case 't':
          ftest = 1;
          break;
        case 'c':
          fcompact = 1;
          break;
        case 'l':
          flog = 1;
          strncpy(path_log, optarg, PATH_MAX);
          break;
        case ':':
          return err(E_MISSARG);
        default:
          return err(E_UNKOPT);
      }
   }
  if (optind < argc)
    return err(E_WRONGARG);

  return E_SUCCESS;
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
  double _u, _i, _frq, _p, _pf;

  cleartrail(buf);
  sscanf(strtok(buf, ";"), "%lf", &_u);
  if (_u < 0.0f)
    u = _u;
  if (u >= 0.0f)
    u += (_u - u) / iters;
  sscanf(strtok(NULL, ";"), "%lf", &_i);
  if (_i < 0.0f)
    i = _i;
  if (i >= 0.0f)
    i += (_i - i) / iters;
  sscanf(strtok(NULL, ";"), "%lf", &_frq);
  if (_frq < 0.0f)
    frq = _frq;
  if (frq >= 0.0f)
    frq += (_frq - frq) / iters;
  sscanf(strtok(NULL, ";"), "%lf", &_p);
  if (_p < 0.0f)
    p = _p;
  if (p >= 0.0f)
  {
    p += (_p - p) / iters;
    e += (double) _p * averaging_interval / 1000;
  }
  sscanf(strtok(NULL, ";"), "%lf", &_pf);
  if (_pf < 0.0f)
    pf = _pf;
  if (pf >= 0.0f)
    pf += (_pf - pf) / iters;
  if (flog)
  {
    if (!strftime(timebuf, BUFSIZE, "%Y-%m-%d %H:%M:%S %Z", localtime(&tt)))
      timebuf[0] = 0;
    fprintf(file_log, "%s; %ld; %f; %f; %f; %f; %f\n", timebuf, tt - tts, _u,
      _i, _frq, _p, _pf);
  }

}

void printresults(void)
{
  memset(timebuf, 0, BUFSIZE);
  time(&tt);
  if (fcompact)
  {
    printf("%lu;%f;%g\n", tt - tts, p, p * (tt - tts) / 3600.0f);
//    fflush(stdout);
  }
  else
  {
    strftime(timebuf, BUFSIZE, "%Y-%m-%d %H:%M:%S %Z", localtime(&tt));
    printf("%s; %ld; %f; %f; %f; %f; %f; %f; calc P: %f\n", timebuf, tt - tts,
      u, i, frq, p, pf, p * (tt - tts) / 3600.0f, e / (tt - tts));
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
  fcompact = 0;
  time(&tts);

  ret = parse_args(argc, argv);
  if (ret < 0)
    return EXIT_SUCCESS;
  if (ret > 0)
    return ret;

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
  sigaction(SIGINT, &act, NULL);
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

  if (ftest)
  {
    close(f);
    printf("Chroma 66202 power meter found and it seems OK.\n");
    return EXIT_SUCCESS;
  }

  if (!fcompact)
  {
    printf("Timestamp; Time [s]; U [V]; I [A]; P [W]; PF [-]; E [Wh]\n");
//    fflush(stdout);
  }

  if (flog)
  {
    if ((file_log = fopen(path_log, "w")))
      setlinebuf(file_log);
    else
    {
      flog = 0;
      fprintf(stderr, "Error: unable to create '%s'.\n", path_log);
    }
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
    rem.tv_sec = averaging_interval / 1000;
    rem.tv_nsec = averaging_interval % 1000 * 1000;
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
  if (flog)
    fclose(file_log);
  close(f);
  return 0;
}
