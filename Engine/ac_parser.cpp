#include "ac_parser.h"

#include "ac.h"
#include "ac_context.h"
#include "ac_string.h"

/* *** SCRIPT SYMBOL: [Parser] SaidUnknownWord *** */
int SaidUnknownWord (char*buffer) {
  VALIDATE_STRING(buffer);
  strcpy (buffer, play.bad_parsed_word);
  if (play.bad_parsed_word[0] == 0)
    return 0;
  return 1;
}

/* *** SCRIPT SYMBOL: [Parser] Parser::SaidUnknownWord^0 *** */
const char* Parser_SaidUnknownWord() {
  if (play.bad_parsed_word[0] == 0)
    return NULL;
  return CreateNewScriptString(play.bad_parsed_word);
}

int is_valid_word_char(char theChar) {
  if ((isalnum(theChar)) || (theChar == '\'') || (theChar == '-')) {
    return 1;
  }
  return 0;
}

int find_word_in_dictionary (char *lookfor) {
  int j;
  if (game.dict == NULL)
    return -1;

  for (j = 0; j < game.dict->num_words; j++) {
    if (stricmp(lookfor, game.dict->word[j]) == 0) {
      return game.dict->wordnum[j];
    }
  }
  if (lookfor[0] != 0) {
    // If the word wasn't found, but it ends in 'S', see if there's
    // a non-plural version
    char *ptat = &lookfor[strlen(lookfor)-1];
    char lastletter = *ptat;
    if ((lastletter == 's') || (lastletter == 'S') || (lastletter == '\'')) {
      *ptat = 0;
      int reslt = find_word_in_dictionary (lookfor);
      *ptat = lastletter;
      return reslt;
    } 
  }
  return -1;
}

int FindMatchingMultiWordWord(char *thisword, char **text) {
  // see if there are any multi-word words
  // that match -- if so, use them
  const char *tempptr = *text;
  char tempword[150] = "";
  if (thisword != NULL)
    strcpy(tempword, thisword);

  int bestMatchFound = -1, word;
  const char *tempptrAtBestMatch = tempptr;

  do {
    // extract and concat the next word
    strcat(tempword, " ");
    while (tempptr[0] == ' ') tempptr++;
    char chbuffer[2];
    while (is_valid_word_char(tempptr[0])) {
      sprintf(chbuffer, "%c", tempptr[0]);
      strcat(tempword, chbuffer);
      tempptr++;
    }
    // is this it?
    word = find_word_in_dictionary(tempword);
    // take the longest match we find
    if (word >= 0) {
      bestMatchFound = word;
      tempptrAtBestMatch = tempptr;
    }

  } while (tempptr[0] == ' ');

  word = bestMatchFound;

  if (word >= 0) {
    // yes, a word like "pick up" was found
    *text = (char*)tempptrAtBestMatch;
    if (thisword != NULL)
      strcpy(thisword, tempword);
  }

  return word;
}

// parse_sentence: pass compareto as NULL to parse the sentence, or
// compareto as non-null to check if it matches the passed sentence
int parse_sentence (char*text, int *numwords, short*wordarray, short*compareto, int comparetonum) {
  char thisword[150] = "\0";
  int  i = 0, comparing = 0;
  char in_optional = 0, do_word_now = 0;
  int  optional_start = 0;

  numwords[0] = 0;
  if (compareto == NULL)
    play.bad_parsed_word[0] = 0;
  strlwr(text);
  while (1) {
    if ((compareto != NULL) && (compareto[comparing] == RESTOFLINE))
      return 1;

    if ((text[0] == ']') && (compareto != NULL)) {
      if (!in_optional)
        quit("!Said: unexpected ']'");
      do_word_now = 1;
    }

    if (is_valid_word_char(text[0])) {
      // Part of a word, add it on
      thisword[i] = text[0];
      i++;
    }
    else if ((text[0] == '[') && (compareto != NULL)) {
      if (in_optional)
        quit("!Said: nested optional words");

      in_optional = 1;
      optional_start = comparing;
    }
    else if ((thisword[0] != 0) || ((text[0] == 0) && (i > 0)) || (do_word_now == 1)) {
      // End of word, so process it
      thisword[i] = 0;
      i = 0;
      int word = -1;

      if (text[0] == ' ') {
        word = FindMatchingMultiWordWord(thisword, &text);
      }

      if (word < 0) {
        // just a normal word
        word = find_word_in_dictionary(thisword);
      }

      // "look rol"
      if (word == RESTOFLINE)
        return 1;
      if (compareto) {
        // check string is longer than user input
        if (comparing >= comparetonum) {
          if (in_optional) {
            // eg. "exit [door]" - there's no more user input
            // but the optional word is still there
            if (do_word_now) {
              in_optional = 0;
              do_word_now = 0;
            }
            thisword[0] = 0;
            text++;
            continue;
          }
          return 0;
        }
        if (word <= 0)
          quitprintf("!Said: supplied word '%s' is not in dictionary or is an ignored word", thisword);
        if (word == ANYWORD) { }
        else if (word != compareto[comparing]) {
          // words don't match - if a comma then a list of possibles,
          // so allow retry
          if (text[0] == ',')
            comparing--;
          else {
            // words don't match
            if (in_optional) {
              // inside an optional clause, so skip it
              while (text[0] != ']') {
                if (text[0] == 0)
                  quit("!Said: unterminated [optional]");
                text++;
              }
              // -1 because it's about to be ++'d again
              comparing = optional_start - 1;
            }
            // words don't match outside an optional clause, abort
            else
              return 0;
          }
        }
        else if (text[0] == ',') {
          // this alternative matched, but there are more
          // so skip the other alternatives
          int continueSearching = 1;
          while (continueSearching) {

            const char *textStart = &text[1];

            while ((text[0] == ',') || (isalnum(text[0]) != 0))
              text++;

            continueSearching = 0;

            if (text[0] == ' ') {
              strcpy(thisword, textStart);
              thisword[text - textStart] = 0;
              // forward past any multi-word alternatives
              if (FindMatchingMultiWordWord(thisword, &text) >= 0)
                continueSearching = 1;
            }
          }

          if ((text[0] == ']') && (in_optional)) {
            // [go,move]  we just matched "go", so skip over "move"
            in_optional = 0;
            text++;
          }

          // go back cos it'll be ++'d in a minute
          text--;
        }
        comparing++;
      }
      else if (word != 0) {
        // it's not an ignore word (it's a known word, or an unknown
        // word, so save its index)
        wordarray[numwords[0]] = word;
        numwords[0]++;
        if (numwords[0] >= MAX_PARSED_WORDS)
          return 0;
        // if it's an unknown word, store it for use in messages like
        // "you can't use the word 'xxx' in this game"
        if ((word < 0) && (play.bad_parsed_word[0] == 0))
          strcpy(play.bad_parsed_word, thisword);
      }

      if (do_word_now) {
        in_optional = 0;
        do_word_now = 0;
      }
  
      thisword[0] = 0;
    }
    if (text[0] == 0)
      break;
    text++;
  }
  // If the user input is longer than the Said string, it's wrong
  // eg Said("look door") and they type "look door jibble"
  // rol should be used instead to enable this
  if (comparing < comparetonum)
    return 0;
  return 1;
}

/* *** SCRIPT SYMBOL: [Parser] Parser::ParseText^1 *** */
/* *** SCRIPT SYMBOL: [Parser] ParseText *** */
void ParseText (char*text) {
  parse_sentence (text, &play.num_parsed_words, play.parsed_words, NULL, 0);
}

/* *** SCRIPT SYMBOL: [Parser] Parser::FindWordID^1 *** */
int Parser_FindWordID(const char *wordToFind)
{
  return find_word_in_dictionary((char*)wordToFind);
}

// Said: call with argument for example "get apple"; we then check
// word by word if it matches (using dictonary ID equivalence to match
// synonyms). Returns 1 if it does, 0 if not.
/* *** SCRIPT SYMBOL: [Parser] Parser::Said^1 *** */
/* *** SCRIPT SYMBOL: [Parser] Said *** */
int Said (char*checkwords) {
  int numword = 0;
  short words[MAX_PARSED_WORDS];
  return parse_sentence (checkwords, &numword, &words[0], play.parsed_words, play.num_parsed_words);
}



void register_parser_script_functions() {
    scAdd_External_Symbol("Parser::FindWordID^1",(void *)Parser_FindWordID);
    scAdd_External_Symbol("Parser::ParseText^1",(void *)ParseText);
    scAdd_External_Symbol("Parser::SaidUnknownWord^0",(void *)Parser_SaidUnknownWord);
    scAdd_External_Symbol("Parser::Said^1",(void *)Said);
    scAdd_External_Symbol("Said",(void *)Said);
    scAdd_External_Symbol("SaidUnknownWord",(void *)SaidUnknownWord);
  scAdd_External_Symbol("ParseText",(void *)ParseText);
}