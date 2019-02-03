/*
  Frontend for measuring energy consumption with ACPI power meter
  Copyright (C) 2019 Yarda <jskarvad@redhat.com>

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
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>

#define PROGNAME acpienergy
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

#define METER_PATH "/sys/bus/acpi/drivers/power_meter"

#define BUFSIZE 1024

int fcompact = 0;
int ftest = 0;
int flog = 0;
volatile int ssigusr1 = 0;
volatile int ssigterm = 0;
unsigned long long iters;
time_t tt, tts;
double p = 0.0f;
double e = 0.0f;
char timebuf[BUFSIZE] = {0};
char meter_path[PATH_MAX] = { 0 };
char mpath[PATH_MAX] = { 0 };
char path_log[PATH_MAX] = { 0 };
// in ms
int averaging_interval = 0;
struct sigaction acts_sigusr1 = {{0}};
struct sigaction acts_sigusr2 = {{0}};
struct sigaction acts_term = {{0}};
struct sigaction act = {{0}};
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
  printf("ACPI power meter frontend.\n");
  printf("Version %s %s\n", VERSION_STRING, COPYRIGHT_STRING);
  printf("This program is distributed under the terms of the ");
  printf("GNU General Public License.\n\n");
  printf("Usage: %s [OPTIONS]\n", argv[0]);
  printf("  -lLOG          Log all readings to the LOG.\n");
  printf("  -c             Compact mode, show only the time, power and ");
  printf("  -t             Test mode, just check presence of the ACPI power ");
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

int find_acpi_power_meter()
{
  int x;
  DIR *d;
  FILE *f;
  struct dirent *de;
  float accuracy;
  float accuracy_best = 0.0f;

  if (!(d = opendir(METER_PATH)))
    return 0;
  while ((de = readdir(d)))
  {
    if (de->d_name && de->d_name[0] != '.')
    {
      for (x = 1; x <= 255; x++)
      {
        if (snprintf(mpath, PATH_MAX, "%s/%s/power%d_accuracy", METER_PATH, de->d_name, x))
          ;
        if ((f = fopen(mpath, "r")))
        {
          if (fscanf(f, "%f", &accuracy) == 1)
          {
            if (accuracy > accuracy_best)
            {
              accuracy_best = accuracy;
              if (snprintf(meter_path, PATH_MAX, "%s/%s/power%d", METER_PATH, de->d_name, x))
                ;
            }
          }
          fclose(f);
        }
        else
          break;
      }
    }
  }
  closedir(d);
  return accuracy_best > 0.0f;
}

int read_averaging_interval()
{
  int ret = 0;
  FILE *f;

  if (snprintf(mpath, PATH_MAX, "%s_average_interval", meter_path))
    ;

  if ((f = fopen(mpath, "r")))
  {
    ret = fscanf(f, "%d", &averaging_interval) == 1;
    fclose(f);
  }
  if (snprintf(mpath, PATH_MAX, "%s_average", meter_path))
    ;
  return ret;
}

int read_acpi_power_meter()
{
  FILE *f;
  int pwr;

  pwr = -1;
  if ((f = fopen(mpath, "r")))
  {
    fscanf(f, "%d", &pwr);
    fclose(f);
  }
// return in W
  return pwr / 1000000;
}

void integr(void)
{
  int pwr;

  time(&tt);
  pwr = read_acpi_power_meter();
  if (pwr >= 0)
  {
    if (p >= 0.0f)
      p += (pwr - p) / iters;
    e += (double) pwr * averaging_interval / 1000;
  }
  if (flog)
  {
    if (!strftime(timebuf, BUFSIZE, "%Y-%m-%d %H:%M:%S %Z", localtime(&tt)))
      timebuf[0] = 0;
    fprintf(file_log, "%s; %ld; %d\n", timebuf, tt - tts, pwr);
  }
}

void printresults(void)
{
  time(&tt);
  if (fcompact)
  {
    printf("%lu;%g;%g\n", tt - tts, p, e / 3600.0f);
//    fflush(stdout);
  }
  else
  {
    if (!strftime(timebuf, BUFSIZE, "%Y-%m-%d %H:%M:%S %Z", localtime(&tt)))
      timebuf[0] = 0;
    printf("%s; %f; %f; calc: %f\n", timebuf, p, e / 3600.0f, e / (tt - tts));
//    fflush(stdout);
  }
}

int main(int argc, char *argv[])
{
  int ret;
  struct timespec rem = {0};

  time(&tts);

  ret = parse_args(argc, argv);
  if (ret < 0)
    return EXIT_SUCCESS;
  if (ret > 0)
    return ret;

  // line buffering stdout
  //setvbuf(stdout, NULL, _IOLBF, 0);
  setlinebuf(stdout);
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

  if (!find_acpi_power_meter())
  {
    fprintf(stderr, "Error: unable to find ACPI power meter.\n");
    return 1;
  }
  read_averaging_interval();
  if (averaging_interval <= 0)
  {
    fprintf(stderr, "Error: invalid averaging interval.\n");
    return 1;
  }

  if (ftest)
  {
    printf("ACPI power meter found and it seems OK.\n");
    return EXIT_SUCCESS;
  }

  p = 0.0f;
  iters = 0;
  ssigusr1 = 0;
  ssigterm = 0;
  act.sa_handler = sig_handler;
  sigaction(SIGUSR1, &act, NULL);
  sigaction(SIGUSR2, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
  if (!fcompact)
  {
    printf("Time; P [W]; E [Wh]\n");
//    fflush(stdout);
  }

  while (!ssigterm)
  {
    iters++;
    integr();
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
  return EXIT_SUCCESS;
}
