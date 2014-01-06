/*
 * This program seaches for thru SQL statements from standard input
 * and finds "CREATE TABLE `tablename`" using the first argument as
 * the tablename and outputs the full SQL statement.
 *
 * Creation Date: December 13th, 2013
 * Author: John Pruett
 *
 * The program ignores comments in slash-asterisk and double-dash format.
 *
 * Example CREATE TABLE statement:
 *
 * CREATE TABLE `actions` (
 *  `aid` varchar(255) NOT NULL DEFAULT '0' COMMENT 'Primary Key: Unique actions ID.',
 *  `type` varchar(32) NOT NULL DEFAULT '' COMMENT 'Action object (ie: node, user, ...)',
 *  `callback` varchar(255) NOT NULL DEFAULT '' COMMENT 'Callback function.',
 *  `parameters` longblob NOT NULL COMMENT 'Parameters to be passed to the callback function.',
 *  `label` varchar(255) NOT NULL DEFAULT '0' COMMENT 'Label of the action.',
 *  PRIMARY KEY (`aid`)
 * ) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Stores action information.';
 *
 * Visual of buffer:
 *
 * 0 1 2 3 4 5 6 7 8 9 A B C D E F ... -> BUFMAXSIZE
 * ^-bufbegin initial point
 * ^-bufend initial point
 * 
 * When replenishing the buffer, buffer will be shifted by BUFSHIFTSIZE size.
 *
 */

#include <stdio.h>
#include <string.h>

#define BUFMAXSIZE 16384  /* 16K */
#define BUFSHIFTSIZE 4096   /* Shift buffer by this amount */
#define BUFCUTOFF BUFMAXSIZE - BUFSHIFTSIZE
#define NAMESIZE 65
#define TABLESIZE "%65s"
#define SEARCHSIZE 128
#define NEWLINE '\n'
#define QUOTE '\''

typedef enum { FALSE, TRUE } bool;

char buffer[BUFMAXSIZE+1]=""; /* Includes the last 0 filled character */
int  bufbegin=0;              /* Begin at zero */
int  bufend=0;                /* Ending position points to the last character plus 1 */
int  bufsize=0;               /* Number of characters currently in buffer */
char table[NAMESIZE]="";      /* Contains the name of the table */
char search[SEARCHSIZE]="";   /* Contains the search string */
int  searchlen=0;             /* Length of the search string */
bool found=FALSE;             /* Is it found??? */
bool searching=TRUE;          /* Continue Searching */
bool inlinecomment=FALSE;     /* Inside inline comment */
bool cstylecomment=FALSE;     /* Inside c-style comment */
bool insidequote=FALSE;       /* Inside single quote */
char lastchar='\n';           /* Last Character observed */

/* Try to fill buffer and return true if the size is non-zero */
bool pushbuffer()
{
  char c=0;       /* Last character read */

  /* printf("Len of Buffer: %d\n",strlen(buffer)); */
  /* If the current buffer size is less than the cut off, and the beginning position
     is larger than the shift size, move the buffer to zero position. */
  if((bufsize<BUFCUTOFF)&&(bufbegin>BUFSHIFTSIZE))
  {
    int i=0;        /* Move characters here. */
    int j=bufbegin; /* Grab characters here. */

    /* printf("59:bufsize(%d)<BUFCUTOFF(%d) and bufbegin(%d)>BUFFSHIFTSIZE(%d) - ", bufsize,BUFCUTOFF,bufbegin,BUFSHIFTSIZE); */
    /* Shift buffer by bufsize */
    while(i<bufsize)
    {
      /* printf("[%d]",i); */
      buffer[i++]=buffer[j++];
    }
    /* printf("\n"); */
    
    bufbegin=0;     /* Buffer starting position is now at zero. */
    bufend=bufsize; /* Buffer ending position is moved by limit. */
  }

  /* If there is something to read and there is space in the buffer,
     place the current character into c and the current buffer end.
     Increment the buffer end by 1 */
  while((c!=EOF)&&(bufend!=BUFMAXSIZE))
  {
    c=getc(stdin);
    if(c!=EOF) buffer[bufend++]=c;
  }

  /* Place an end-of-line character at the end of the new buffer */
  buffer[bufend]='\0';

  /* Reset the buffer size */
  bufsize=bufend-bufbegin;

  /* printf("78:Buffer '%s' has %d characters and begins at %d - ",buffer,bufsize,bufbegin); */
  return (bufsize!=0);  /* True if there are characters in the buffer */
}

/* If there is something in the buffer output the next character */
char popbuffer()
{
  if(bufsize>0)
  {
    /* printf("Character: %d=(%d)",bufbegin,buffer[bufbegin]); */
    /* putc('\n', stdout); */
    bufsize--;
    return buffer[bufbegin++];
  }
}

void searchbuffer()
{
  char c;
  
  c=buffer[bufbegin]; /* Current Character */
  
  /* Check for quote */
  if(!inlinecomment&&!cstylecomment&&!insidequote&&c==QUOTE)
  {
    /* printf("Inside quote\n"); */
    insidequote=TRUE;
  }

  /* Check for c-style comment */
  else if(!insidequote&&!inlinecomment&&strncmp(&buffer[bufbegin],"/*",2)==0)
  {
    /* printf("C-Style comment\n"); */
    cstylecomment=TRUE;
  }

  /* Check for inline comments */
  else if(!insidequote&&!cstylecomment&&strncmp(&buffer[bufbegin],"--",2)==0)
  {
    /* printf("In-line comment\n"); */
    inlinecomment=TRUE;
  }

  /* printf("Searching in: %s maxsize: %d\n", &buffer[bufbegin],searchlen); */
  else if(insidequote) /* Then look for end of line */
  {
    if(c==QUOTE) insidequote=FALSE;
  }
  else if(cstylecomment) /* Then look for end of line */
  {
    if(strncmp(&buffer[bufbegin],"*/",2)==0) cstylecomment=FALSE;
  }
  else if(inlinecomment) /* Then look for end of line */
  {
    if(c==NEWLINE) inlinecomment=FALSE;
  }
  else if(searching&&strncmp(&buffer[bufbegin],search,searchlen)==0)
  {
    /* printf("FOUND!\n"); */
    found=TRUE;
    searching=FALSE;
  }
  else if(found&&lastchar==';')
  {
    putc(NEWLINE, stdout);
    found=FALSE;
  }
  lastchar=c;
}

/* Run the program..... */
int main(int argc, char *argv[])
{  
  bool runprogram=TRUE;
  char n;
  if(argc==2)
  {
    sscanf(argv[1], TABLESIZE, table);
    snprintf(search, sizeof search, "CREATE TABLE `%s` (", table);
    searchlen=strlen(search);
  } 
  else
  {
    printf ("Usage %s tablename. < file\n",argv[0]);
    runprogram=FALSE;
  } 

  /* Read characters into buffer and write characters from buffer. */
  if (runprogram)
  {
    while(pushbuffer())
    {
      searchbuffer();
      if(found)
      {
        putc(popbuffer(), stdout);
      } else {
        n=popbuffer();
      }
    }
  }
}

