/*
  Frontend for quering Chroma 66202 power meter through USBTMC.
  Copyright (C) 2010-2016 Yarda <jskarvad@redhat.com>

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
#include <limits.h>
#include <getopt.h>
/*#include <sys/ioctl.h>*/
#include <sys/file.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>

#define PROGNAME chroma66202
#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define STRING(S) STR_HELPER(S)
#define STR_HELPER(S) #S
#define PROGNAME_STRING STRING(PROGNAME)
#define VERSION_STRING STRING(VERSION_MAJOR) "." STRING(VERSION_MINOR)
#define COPYRIGHT_STRING \
"Copyright (C) 2010-2016 Yarda <jskarvad@redhat.com>"

#define DEFAULT_DEVICE "/dev/usbtmc0"
#define DEFAULT_WINDOW "1.0"
#define DEFAULT_REPEAT 1
#define DEFAULT_DELAY 10

#define E_SUCCESS   0
#define E_MISSARG   1
#define E_UNKOPT    2
#define E_WRONGARG  3
#define E_COMM      4
#define E_LOCK      5
#define E_OPEN      6
#define E_INIT      7

#define BUFSIZE 1024

/* Let the device some time to process the command (in usec) */
#define DELAY 1000

#define cr(arg) \
(command(f, (arg)) && response(f, buf, BUFSIZE))

#define c(arg) \
command(f, (arg))

#define cp(arg1, arg2) \
(strncpy(buf, (arg1), BUFSIZE - 1), \
  strncat(buf, (arg2), BUFSIZE - 1 - strlen(buf)), \
  strncat(buf, "\n", BUFSIZE - 1 - strlen(buf)), \
command(f, buf))

#define header_add(header, str) \
csv_add((header), (str), ';')

#define meas_add(header, meas, hstr, mstr) \
{\
  header_add((header), (hstr));\
  csv_add((meas), (mstr), ',');\
}

char device[PATH_MAX] = DEFAULT_DEVICE;
char buf[BUFSIZE] = {0};
char timebuf[BUFSIZE] = {0};
char header[256] = {0};
char meas[256] = {0};
char *mtype, *model, *serial, *firmware;
int f;
time_t tt;
struct tm tm;

volatile int ssigusr1 = 0;
volatile int ssigusr2 = 0;
// whether exclusively lock the device
int flockdev = 1;
int fsig = 0;
int fint = 0;
int finfo = 0;
int fnocsvhead = 0;
int fu = 0;
int fi = 0;
int ff = 0;
int fp = 0;
int fpf = 0;
int ft = 0;
int fr = 0;
int repeat = DEFAULT_REPEAT;
int delay = DEFAULT_DELAY;
int tintegratei = 0;
char shunt[5] = "AUTO";
char irange[5] = "0";
char urange[5] = "AUTO";
char *win;
char window[6] = DEFAULT_WINDOW;
char tintegrate[5] = "0";
/*
struct sigaction acts_sigusr1 = {0};
struct sigaction acts_sigusr2 = {0};
struct sigaction acts_term = {0};
*/

struct option long_options[] = {
  {"help", 0, 0, 'h'},
  {"version", 0, 0, 'v'},
  {0, 0, 0, 0}
};


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
  printf("Chroma 66202 power meter USBTMC frontend.\n");
  printf("Version %s %s\n", VERSION_STRING, COPYRIGHT_STRING);
  printf("This program is distributed under the terms of the ");
  printf("GNU General Public License.\n\n");
  printf("Usage: %s [OPTIONS]\n", argv[0]);
  printf("  -dDEVICE       Specify device (default %s)\n", DEFAULT_DEVICE);
  printf("  -l             display info about connected device.\n");
  printf("  -k             Do not lock the device for exclusive access (not ");
  printf("recommended).\n");
  printf("                 It uses advisory locking through flock, thus ");
  printf("other tools\n");
  printf("                 non-compatible with the flock may ignore the ");
  printf("exclusive lock\n");
  printf("                 even if not disabled by this option.\n");
  printf("  -e             Perform reset (*RST command) before issuing \n");
  printf("                 any other command. Use in case of troubles.\n");
  printf("  -sSHUNT        Shunt range, can be: h, l, a, ");
  printf("(default a - auto).\n");
  printf("  -i[RANGE]      Measure RMS current (in A), range can be:\n");
  printf("                 for l shunt: 0, 0.01, 0.1, 0.4, 2, (0 - auto),\n");
  printf("                 for h shunt: 0, 0.2, 2, 8, 20, (0 - auto).\n");
  printf("  -u[RANGE]      Measure RMS voltage (in V), range can be:\n");
  printf("                 0, 150, 300, 500, (default 0 - auto).\n");
  printf("                 If you are over-range you will get -3 value in ");
  printf("results.\n");
  printf("  -p             Measure real power in W.\n");
  printf("  -f             Measure frequency in HZ.\n");
  printf("  -o             Measure power factor.\n");
  printf("  -a             Measure all values.\n");
  printf("  -t             Output timestamp of measurement.\n");
  printf("  -wSIZE         Size of measurement window 0.0 - 10.0 sec, ");
  printf("(default %s sec).\n", DEFAULT_WINDOW);
  printf("  -gTIME         Integrate for number of seconds (accumulated ");
  printf("energy method),\n");
  printf("                 max 9999 seconds. Do not use auto ranges, ");
  printf("otherwise\n");
  printf("                 you can get \"range change error\" (-2 value ");
  printf("in results).\n");
  printf("                 In case of 0 integrate forever, end on SIG_TERM,");
  printf("SIG_USR2,\n");
  printf("                 output so far integrated results on SIG_USR1.\n");
  printf("  -rN            Repeat measurement N times (default %u). ",
         DEFAULT_REPEAT);
  printf("Negative values\n");
  printf("                 means infinite repeat.\n");
  printf("  -yT            Delay between measurements T secs (default %u).\n",
         DEFAULT_DELAY);
  printf("  -c             Do not print CSV headers.\n");
  printf("  --help, -h     Show this help.\n");
  printf("  --version, -v  Show version of this program.\n\n");
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
    case E_COMM:
      str = "Error during communication.";
      break;
    case E_LOCK:
      str = "Error creating lock.";
      break;
    case E_OPEN:
      str = "Error opening file.";
      break;
    case E_INIT:
      str = "Error initalizing device.";
      break;
    default:
      str = "Unknown error.";
  }
  fprintf(stderr, "%s\n", str);
  return code;
}

/*
void sig_handler(int sig)
{
  switch (sig)
  {
    case SIG_USR1:
      ssigusr1 = 1;
      break;
    case SIG_USR2:
    case SIG_TERM:
      ssigusr2 = 1;
  }
}
*/

int parse_args(int argc, char *argv[])
{
  char c1, c2;
  int x, opt, option_index;
  float r;

  while ((opt =
          getopt_long (argc, argv, "aetld:nk:s:i::u::pw:g:r:y:chv", long_options,
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
        case 'k':
          flockdev = 0;
          break;
        case 'd':
          strncpy(device, optarg, PATH_MAX);
          break;
        case 'l':
          finfo = 1;
          break;
        case 'e':
          fr = 1;
          break;
        case 'c':
          fnocsvhead = 1;
          break;
        case 's':
          if (sscanf(optarg, "%c%c", &c1, &c2) != 1)
            return err(E_WRONGARG);
          switch (toupper(c1))
          {
            case 'L':
              strcpy(shunt, "LOW");
              break;
            case 'H':
              strcpy(shunt, "HIGH");
              break;
            case 'A':
              strcpy(shunt, "AUTO");
              break;
            default:
              return err(E_WRONGARG);
          }
          break;
        case 'u':
          fu = 1;
          if (optarg != NULL)
          {
            sscanf(optarg, "%s", (char *) &urange);
            if (!strcmp(urange, "150"))
              strcpy(urange, "V150");
            else if (!strcmp(urange, "300"))
              strcpy(urange, "V300");
            else if (!strcmp(urange, "500"))
              strcpy(urange, "V500");
            else if (!strcmp(urange, "0"))
              strcpy(urange, "AUTO");
            else return err(E_WRONGARG);
          }
          break;
        case 'i':
          fi = 1;
          if (optarg != NULL)
            sscanf(optarg, "%s", (char *) &irange);
          break;
        case 'f':
          ff = 1;
          break;
        case 'p':
          fp = 1;
          break;
        case 'o':
          fpf = 1;
          break;
        case 'a':
          fu = fi = fp = ff = fpf = 1;
          break;
        case 't':
          ft = 1;
          break;
        case 'w':
          if (sscanf(optarg, "%f%c", &r, &c1) != 1 || r < 0 || r > 10)
            return err(E_WRONGARG);
          snprintf(window, 6, "%4.1f0", r);
          win = window;
          while (*win == ' ') win++;
          break;
        case 'g':
          if (sscanf(optarg, "%u%c", (unsigned int *) &x, &c1) != 1 ||
              x < 0 || x > 9999)
            return err(E_WRONGARG);
          fint = 1;
          if (!x)
          {
            fsig = 1;
            x = 1;
          }
          tintegratei = x;
          snprintf(tintegrate, 5, "%u", x);
          break;
        case 'r':
          if (sscanf(optarg, "%d%c", &repeat, &c1) != 1)
            return err(E_WRONGARG);
          break;
        case 'y':
          if (sscanf(optarg, "%u%c", &delay, &c1) != 1 || delay < 0)
            return err(E_WRONGARG);
          break;
        case ':':
          return err(E_MISSARG);
        default:
          return err(E_UNKOPT);
      }
   }
  if (optind < argc)
    return err(E_WRONGARG);

  if (argc == 1)
  {
    help(argv);
    return -1;
  }

  return E_SUCCESS;
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

int checkirange()
{
  int b = 0;

  if (!strcmp(shunt, "LOW") || !strcmp(shunt, "AUTO"))
  {
    if (!strcmp(irange, "0.01"))
      strcpy(irange, "A001");
    else if (!strcmp(irange, "0.1"))
      strcpy(irange, "A01");
    else if (!strcmp(irange, "0.4"))
      strcpy(irange, "A04");
    else if (!strcmp(irange, "2"))
      strcpy(irange, "A2L");
    else if (!strcmp(irange, "0"))
      strcpy(irange, "AUTO");
    else if (strcmp(shunt, "AUTO"))
      return err(E_WRONGARG);
    else b = 1;
  }

  if (!strcmp(shunt, "HIGH") || !strcmp(shunt, "AUTO"))
  {
    if (!strcmp(irange, "0.2"))
      strcpy(irange, "A02");
    else if (!strcmp(irange, "2"))
      strcpy(irange, "A2H");
    else if (!strcmp(irange, "8"))
      strcpy(irange, "A8");
    else if (!strcmp(irange, "20"))
      strcpy(irange, "A20");
    else if (!strcmp(irange, "0"))
      strcpy(irange, "AUTO");
    else if (strcmp(shunt, "AUTO") || b)
      return err(E_WRONGARG);
  }
  return E_SUCCESS;
}

void csv_add(char *csv, char *str, char sep)
{
  int x;

  if ((x = strlen(csv)))
  {
    csv[x] = sep;
    csv[x + 1] = 0;
  }
  strcat(csv, str);
}

int init()
{
  char *str;
/*
  struct sigaction act = {0};
*/

  if ((f = open(device, O_RDWR)) == -1)
  {
    printf("Error opening device '%s': %s\n", device, strerror(errno));
    return 0;
  }

  if (flockdev)
    if (flock(f, LOCK_EX | LOCK_NB) < 0)
    {
      fprintf(stderr, "Unable to obtain lock on device '%s' for exclusive "
        "access.\n", device);
      close(f);
      return E_LOCK;
    }

  if (fr)
  {
    if (!c("*RST\n"))
    {
      close(f);
      return E_COMM;
    }
  }

  if (!c("SYST:HEAD OFF\n") ||
      !c("SYST:TRAN:SEP 1\n") || !c("SYST:TRAN:TERM 0\n") ||
      !c("SOUR:POW:INT 0\n") ||
      !cp("SOUR:CURR:SHUN ", shunt) || !c("SOUR:MEAS:MODE WINDOW\n") ||
      !cp("SOUR:MEAS:WIND ", win) || !c("SOUR:TRIG:MODE NONE\n") ||
      !cp("SOUR:CURR:RANG ", irange) || !cp("SOUR:VOLT:RANG ", urange) ||
      !cp("SOUR:POW:INT ", tintegrate) || !cr("*IDN?"))
  {
    close(f);
    return E_COMM;
  }

  if (ft)
    header_add(header, "Date/Time"); 
  if (fu)
    meas_add(header, meas, "U", "V");
  if (fi)
    meas_add(header, meas, "I", "I");
  if (ff)
    meas_add(header, meas, "f", "FREQ");
  if (fp)
    meas_add(header, meas, "P", "W");
  if (fpf)
    meas_add(header, meas, "pf", "PF");

/*
  if (fsig)
  {
    repeat = 1;
    fsig = 1;
    ssigusr1 = 0;
    ssigusr2 = 0;
    act.sa_handler = sig_handler;
    sigaction(SIG_USR1, &act, &acts_sigusr1);
    sigaction(SIG_USR2, &act, &acts_sigusr2);
    sigaction(SIG_TERM, &act, &acts_sigterm);
  }
*/
  str = buf;
  mtype = strtok(str, ",");
  model = strtok(NULL, ",");
  serial = strtok(NULL, ",");
  firmware = strtok(NULL, ",");
  return mtype && !strcmp("Chroma ATE", mtype) && model &&
         !strcmp("66202", model);
}

void done()
{
  c("SOUR:POW:INT 0\n");
  if (flockdev)
      flock(f, LOCK_UN);

  close(f);
/*
  if (fsig)
  {
    sigaction(SIG_USR1, &acts_sigusr1);
    sigaction(SIG_USR2, &acts_sigusr2);
    sigaction(SIG_TERM, &acts_sigterm);
  }
*/
}

int do_info()
{
  printf("Type     : %s\nModel    : %s\nSerial   : %s\nFirmware : %s\n",
         mtype, model, serial, firmware);
  return E_SUCCESS;
}

void cleartrail(char *buf)
{
  int x;

  x = strlen(buf);
  while (x-- && (buf[x] == '\r' || buf[x] == '\n'))
    buf[x] = 0;
}

int process()
{
  int infinite_repeat;

  if (finfo)
    return do_info();

  if (!fnocsvhead)
    printf("%s\n", header);

  infinite_repeat = repeat < 0;
  while (infinite_repeat || repeat--)
  {
    if (fint)
    {
      if (!cp("MEAS? ", meas) || !response(f, buf, BUFSIZE) ||
          sleep(tintegratei))
        return err(E_COMM);
      while (!strncmp(buf, "-1", 2))
      {
        sleep(1);
        if (!cp("FETC? ", meas) || !response(f, buf, BUFSIZE))
          return err(E_COMM);
      }
    }
    else
    {
      if (!cp("MEAS? ", meas) || !response(f, buf, BUFSIZE))
        return err(E_COMM);
    }
    if (ft)
    {
      memset(timebuf, 0, BUFSIZE);
      time(&tt);
      strftime(timebuf, BUFSIZE, "%Y-%m-%d %H:%M:%S %Z", localtime(&tt));
      printf("%s;", timebuf);
    }
    cleartrail(buf);
    printf("%s\n", buf);
    if (repeat && delay)
      sleep(delay);
  }

  return E_SUCCESS;
}

int main(int argc, char *argv[])
{
  int ret;

  // line buffering stdout
  setvbuf(stdout, NULL, _IOLBF, 0);
  win = window;
  ret = parse_args(argc, argv);
  if (ret < 0)
    return EXIT_SUCCESS;
  if (ret > 0)
    return ret;

  if ((ret = checkirange()))
    return ret;

  if (init())
  {
    ret = process();
    done();
  }
  else
    ret = err(E_INIT);

  return ret;
}
