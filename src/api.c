#include "api.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static MateConfig mateState = {0};

/* --- Build System Implementation --- */
static void mateSetDefaultState(void) {
  mateState.arena = ArenaCreate(20000 * sizeof(String));
  mateState.compiler = GetCompiler();

  mateState.mateExe = mateFixPathExe(S("./mate"));
  mateState.mateSource = mateFixPath(S("./mate.c"));
  mateState.buildDirectory = mateFixPath(S("./build"));
}

static MateConfig mateParseMateConfig(MateOptions options) {
  MateConfig result = {0};
  result.compiler = options.compiler;
  result.mateExe = StrNew(mateState.arena, options.mateExe);
  result.mateSource = StrNew(mateState.arena, options.mateSource);
  result.buildDirectory = StrNew(mateState.arena, options.buildDirectory);
  return result;
}

void CreateConfig(MateOptions options) {
  mateSetDefaultState();
  MateConfig config = mateParseMateConfig(options);

  if (!StrIsNull(config.mateExe)) {
    mateState.mateExe = mateFixPathExe(config.mateExe);
  }

  if (!StrIsNull(config.mateSource)) {
    mateState.mateSource = mateFixPath(config.mateSource);
  }

  if (!StrIsNull(config.buildDirectory)) {
    mateState.buildDirectory = mateFixPath(config.buildDirectory);
  }

  if (config.compiler != 0) {
    mateState.compiler = config.compiler;
  }

  mateState.initConfig = true;
}

static void mateReadCache(void) {
  String mateCachePath = F(mateState.arena, "%s/mate-cache.ini", mateState.buildDirectory.data);
  errno_t err = IniParse(mateCachePath, &mateState.cache);
  Assert(err == SUCCESS, "MateReadCache: failed reading MateCache at %s, err: %d", mateCachePath.data, err);

  mateState.mateCache.lastBuild = IniGetLong(&mateState.cache, S("modify-time"));
  if (mateState.mateCache.lastBuild == 0) {
    mateState.mateCache.firstBuild = true;
    mateState.mateCache.lastBuild = TimeNow() / 1000;

    String modifyTime = F(mateState.arena, FMT_I64, mateState.mateCache.lastBuild);
    IniSet(&mateState.cache, S("modify-time"), modifyTime);
  }

#if defined(PLATFORM_WIN)
  if (mateState.mateCache.firstBuild) {
    errno_t ninjaCheck = RunCommand(S("ninja --version > nul 2> nul"));
    Assert(ninjaCheck == SUCCESS, "MateReadCache: Ninja build system not found. Please install Ninja and add it to your PATH.");
  }
#else
  mateState.mateCache.samuraiBuild = IniGetBool(&mateState.cache, S("samurai-build"));
  if (mateState.mateCache.samuraiBuild == false) {
    Assert(mateState.mateCache.firstBuild, "MateCache: This is not the first build and samurai is not compiled, could be a cache error, delete `./build` folder and rebuild `./mate.c`");

    String samuraiAmalgam = s(SAMURAI_AMALGAM);
    String sourcePath = F(mateState.arena, "%s/samurai.c", mateState.buildDirectory.data);
    errno_t errFileWrite = FileWrite(sourcePath, samuraiAmalgam);
    Assert(errFileWrite == SUCCESS, "MateReadCache: failed writing samurai source code to path %s", sourcePath.data);

    String outputPath = F(mateState.arena, "%s/samurai", mateState.buildDirectory.data);
    String compileCommand = F(mateState.arena, "%s \"%s\" -o \"%s\" -std=c99", CompilerToStr(mateState.compiler).data, sourcePath.data, outputPath.data);

    errno_t err = RunCommand(compileCommand);
    Assert(err == SUCCESS, "MateReadCache: Error meanwhile compiling samurai at %s, if you are seeing this please make an issue at github.com/TomasBorquez/mate.h", sourcePath.data);

    LogSuccess("Successfully compiled samurai");
    mateState.mateCache.samuraiBuild = true;
    IniSet(&mateState.cache, S("samurai-build"), S("true"));
  }
#endif

  err = IniWrite(mateCachePath, &mateState.cache);
  Assert(err == SUCCESS, "MateReadCache: Failed writing cache, err: %d", err);
}

static int mateParserError(const char *error) {
  Assert(error != NULL, "String pointer is NULL");

  // TODO: free memory on errors

  LogError("mateArgParser: %s", error);
  return -1;
}

static VectorU32 mateParserScanSeperators(const String str) {
  const char seperators[] = {'=', ','};
  VectorU32 positions = {0};

  for (size_t i = 0; i < str.length; i++) {
    const char cmpChar = str.data[i];

    for (size_t j = 0; j < sizeof(seperators) / sizeof(*seperators); j++) {
      const char sepChar = seperators[j];

      if (sepChar == cmpChar) VecPush(positions, i);
    }
  }

  return positions;
}

static size_t mateParserFindNearestSeperator(const String currentWord, const size_t currentPosition) {
  const VectorU32 sepPositions = mateParserScanSeperators(currentWord);

  size_t nearestSep = currentWord.length;
  for (size_t k = 0; k < sepPositions.length; k++) {
    const u32 sepPosition = sepPositions.data[k];

    if (currentPosition <= sepPosition && sepPosition < nearestSep) nearestSep = sepPosition;
  }

  // sanity check
  Assert(nearestSep >= currentPosition, "Math ain't mathing");

  return nearestSep;
}

static size_t mateParserFindMatchingFlagIndex(ArgParserFlagVector *targetVector, const String *const tokenStr) {
  for (size_t i = 0; i < targetVector->length; i++) {
    const ArgParserFlag flag = VecAt(*targetVector, i);
    if (StrEq(flag.name, *tokenStr)) return i;
  }
  return -1;
}

static const char *mateParserTypeToCStr(argParserDataType dataType) {
  switch (dataType) {
  default:
    return "Unknown";
  case ARG_PARSER_DATA_TYPE_VOID:
    return "Void";
  case ARG_PARSER_DATA_TYPE_INT:
    return "Int";
  case ARG_PARSER_DATA_TYPE_FLOAT:
    return "Float";
  case ARG_PARSER_DATA_TYPE_BOOL:
    return "Bool";
  case ARG_PARSER_DATA_TYPE_STRING:
    return "String";
  }
}

/// returns 0 on success
static int mateParseArguments(int argc, char **argv) {
  mateState.argParserState.stringArena = ArenaCreate(8);

  ArgParserTokenVector tokenVec = {0};

  if (argc < 1) {
    return mateParserError("Invalid argc!");
  } else if (argv == NULL) return mateParserError("Invalid argv!");

  // lexer
  for (int wordIdx = 1; wordIdx < argc; wordIdx++) {
    const String currentWord = s(argv[wordIdx]);
    VectorU32 sepPositions = mateParserScanSeperators(currentWord);

    for (size_t tokenIdx = 0; tokenIdx < currentWord.length; tokenIdx++) {
      ArgParserTokenData tokenData = {.data.str = S(""), .type = ARG_PARSER_TOKEN_INVALID};

      const char currentToken = currentWord.data[tokenIdx];
      size_t nearestSep = tokenIdx;

      switch (currentToken) {
      case ' ':
        break;
      case '-':
        switch (currentWord.data[tokenIdx + 1]) {
        case 'D':
          tokenData.type = ARG_PARSER_TOKEN_USER_FLAG;

          if (currentWord.length <= 2) {
            tokenData.data.str = S("");
            tokenIdx = currentWord.length - 1;
            continue;
          }
          tokenIdx += 2;
          nearestSep = mateParserFindNearestSeperator(currentWord, tokenIdx);
          tokenData.data.str = StrNewSize(mateState.argParserState.stringArena, currentWord.data + tokenIdx, nearestSep - tokenIdx);

          // next word
          tokenIdx = nearestSep - 1;
          break;
        default:
          // try to convert it to a negative number, if it fails it must be a flag
          ; // weird C standard thingy or you might get some errors on some compilers (Label followed by a declaration is a C23 extension)
          char *end = NULL;
          const double value = strtod(currentWord.data + tokenIdx, &end);

          nearestSep = mateParserFindNearestSeperator(currentWord, tokenIdx);

          if (end == currentWord.data + nearestSep) { // if it's a valid number
            tokenData.type = ARG_PARSER_TOKEN_NUMBER;
            tokenData.data.float64 = value;
          } else {
            tokenData.type = ARG_PARSER_TOKEN_FLAG;
            tokenIdx++;
            tokenData.data.str = StrNewSize(mateState.argParserState.stringArena, currentWord.data + tokenIdx, nearestSep - tokenIdx);
          }

          tokenIdx = nearestSep - 1;
          break;
        }
        break;
      case '=':
        tokenData.data.str = S("=");
        tokenData.type = ARG_PARSER_TOKEN_EQUAL;
        break;
      case ',':
        tokenData.data.str = S(",");
        tokenData.type = ARG_PARSER_TOKEN_COMMA;
        break;
      default:
        if (currentToken == '"') {
          // try finding the next "
          u32 k = tokenIdx;
          while (currentWord.data[++k] != '"')
            if (currentWord.data[k] == '\0' || k > currentWord.length) {
              return mateParserError("Invalid string!");
            }

          tokenData.data.str = StrNewSize(mateState.argParserState.stringArena, currentWord.data + tokenIdx + 1, k - tokenIdx - 1);

          tokenData.type = ARG_PARSER_TOKEN_STRING;

          tokenIdx = k;                    // goto the end of the string
        } else if (currentToken == '\'') { // same thing but for ' instead of "
          // try finding the next '
          u32 k = tokenIdx;
          while (currentWord.data[++k] != '\'')
            if (currentWord.data[k] == '\0' || k > currentWord.length) {
              return mateParserError("Invalid string!");
            }

          tokenData.data.str = StrNewSize(mateState.argParserState.stringArena, currentWord.data + tokenIdx + 1, k - tokenIdx - 1);

          tokenData.type = ARG_PARSER_TOKEN_STRING;

          tokenIdx = k; // goto the end of the string
        } else if (strncmp(currentWord.data + tokenIdx, "true", Min(2, currentWord.length)) == 0 && currentWord.length >= strlen("true")) {
          tokenData.type = ARG_PARSER_TOKEN_BOOL;
          tokenData.data.boolean = true;
          tokenIdx += strlen("true") - 1;
        } else if (strncmp(currentWord.data + tokenIdx, "false", Min(2, currentWord.length)) == 0 && currentWord.length >= strlen("false")) {
          tokenData.type = ARG_PARSER_TOKEN_BOOL;
          tokenData.data.boolean = false;
          tokenIdx += strlen("false") - 1;
        } else {
          char *end = NULL;
          const double value = strtod(currentWord.data + tokenIdx, &end);

          nearestSep = mateParserFindNearestSeperator(currentWord, tokenIdx);

          if (end == currentWord.data + nearestSep) { // if it's a valid number
            tokenData.type = ARG_PARSER_TOKEN_NUMBER;
            tokenData.data.float64 = value;
          } else {
            tokenData.type = ARG_PARSER_TOKEN_STRING;
            tokenData.data.str = StrNewSize(mateState.argParserState.stringArena, currentWord.data + tokenIdx, nearestSep - tokenIdx);
          }

          tokenIdx = nearestSep - 1;
        }
        break;
      }

      VecPush(tokenVec, tokenData);
    }

    if (sepPositions.data) VecFree(sepPositions);
  }

  // debug some info (TODO: delete this)
  if (tokenVec.data)
    for (size_t i = 0; i < tokenVec.length; i++) {
      const ArgParserTokenData tokenData = VecAt(tokenVec, i);

      char *name = "TOK_INVALID";
      switch (tokenData.type) {
      default:
      case ARG_PARSER_TOKEN_INVALID:
        name = "TOK_INVALID";
        LogInfo("%s(%s) ", name, tokenData.data.str.data);
        break;
      case ARG_PARSER_TOKEN_FLAG:
        name = "TOK_FLAG_SHORT";
        LogInfo("%s(%s) ", name, tokenData.data.str.data);
        break;
      case ARG_PARSER_TOKEN_USER_FLAG:
        name = "TOK_FLAG_OPTION";
        LogInfo("%s(%s) ", name, tokenData.data.str.data);
        break;
      case ARG_PARSER_TOKEN_EQUAL:
        name = "TOK_EQUAL";
        LogInfo("%s(%s) ", name, tokenData.data.str.data);
        break;
      case ARG_PARSER_TOKEN_COMMA:
        name = "TOK_COMMA";
        LogInfo("%s(%s) ", name, tokenData.data.str.data);
        break;
      case ARG_PARSER_TOKEN_NUMBER:
        name = "TOK_NUMBER";
        LogInfo("%s(%f) ", name, tokenData.data.float64);
        break;
      case ARG_PARSER_TOKEN_STRING:
        name = "TOK_STRING";
        LogInfo("%s(%s) ", name, tokenData.data.str.data);
        break;
      case ARG_PARSER_TOKEN_BOOL:
        if (tokenData.data.boolean) name = "TOK_BOOL_TRUE";
        else name = "TOK_BOOL_FALSE";
        LogInfo("%s(%s) ", name, tokenData.data.boolean ? "true" : "false");
        break;
      }
    }
  for (int i = 1; i < argc; i++) {
    printf("`%s` ", argv[i]);
  }
  printf("\n");

  // parser
  ArgParserFlagVector userOptionVec = {0};
  ArgParserFlagVector mateOptionVec = {0};

  ArgParserFlagVector *targetVector = NULL;
  size_t targetIdx = 0;
  for (size_t tokenIdx = 0; tokenIdx < tokenVec.length; tokenIdx++) {
    const ArgParserTokenData tokenData = VecAt(tokenVec, tokenIdx);

    switch (tokenData.type) {
    default:
    case ARG_PARSER_TOKEN_INVALID:
      return mateParserError("Invalid argument!");
      break;
    case ARG_PARSER_TOKEN_FLAG:
      Assert(tokenData.data.str.data != NULL && tokenData.data.str.length > 0, "Invalid string!");
      targetVector = &mateOptionVec;

      // check for duplicate options
      targetIdx = mateParserFindMatchingFlagIndex(targetVector, &tokenData.data.str);
      if (targetIdx >= 0) continue;

      VecPush(*targetVector, ((ArgParserFlag){.name = tokenData.data.str, .flagDataVec = {0}}));
      targetIdx = targetVector->length - 1;
      break;
    case ARG_PARSER_TOKEN_USER_FLAG:
      Assert(tokenData.data.str.data != NULL && tokenData.data.str.length > 0, "Invalid string!");
      targetVector = &userOptionVec;

      // check for duplicate options
      targetIdx = mateParserFindMatchingFlagIndex(targetVector, &tokenData.data.str);
      if (targetIdx >= 0) continue;

      VecPush(*targetVector, ((ArgParserFlag){.name = tokenData.data.str, .flagDataVec = {0}}));
      targetIdx = targetVector->length - 1;
      break;
    case ARG_PARSER_TOKEN_EQUAL:
    case ARG_PARSER_TOKEN_COMMA:
      // ignore
      break;
    case ARG_PARSER_TOKEN_NUMBER:
      if (targetVector == NULL) {
        LogWarn("mateArgParser: Unassigned value: `%f`, ignoring...", tokenData.data.float64);
        break;
      }
      VecPush(targetVector->data[targetIdx].flagDataVec, ((ArgParserFlagData){.data = tokenData.data, .dataType = ARG_PARSER_DATA_TYPE_FLOAT}));
      break;
    case ARG_PARSER_TOKEN_STRING:
      if (targetVector == NULL) {
        LogWarn("mateArgParser: Unassigned value: `%s`, ignoring...", tokenData.data.str.data);
        break;
      }
      VecPush(targetVector->data[targetIdx].flagDataVec, ((ArgParserFlagData){.data = tokenData.data, .dataType = ARG_PARSER_DATA_TYPE_STRING}));
      break;
    case ARG_PARSER_TOKEN_BOOL:
      if (targetVector == NULL) {
        LogWarn("mateArgParser: Unassigned value: `%s`, ignoring...", tokenData.data.boolean ? "true" : "false");
        break;
      }
      VecPush(targetVector->data[targetIdx].flagDataVec, ((ArgParserFlagData){.data = tokenData.data, .dataType = ARG_PARSER_DATA_TYPE_BOOL}));
      break;
    }
  }

  // debug some more info
  for (int select = 0; select < 2; select++) {
    targetVector = select == 0 ? &userOptionVec : &mateOptionVec;
    for (size_t i = 0; i < targetVector->length; i++) {
      ArgParserFlag flag = VecAt(*targetVector, i);
      printf("%s flag %s: ", select == 0 ? "user" : "mate", flag.name.data);
      for (size_t j = 0; j < flag.flagDataVec.length; j++) {
        const ArgParserFlagData flagData = VecAt(flag.flagDataVec, j);
        switch (flagData.dataType) {
        default:
        case ARG_PARSER_DATA_TYPE_VOID:
          printf("INVALID ");
          break;
        case ARG_PARSER_DATA_TYPE_INT:
          printf("%d ", (int)flagData.data.int64);
          break;
        case ARG_PARSER_DATA_TYPE_FLOAT:
          printf("%f ", flagData.data.float64);
          break;
        case ARG_PARSER_DATA_TYPE_BOOL:
          printf("%s ", flagData.data.boolean ? "true" : "false");
          break;
        case ARG_PARSER_DATA_TYPE_STRING:
          printf("%s ", flagData.data.str.data);
          break;
        }
      }
      printf("\n");
    }
  }

  // free the token vector
  if (tokenVec.data) VecFree(tokenVec);

  // set the variables in mate state
  mateState.argParserState.userOptionVec = userOptionVec;
  mateState.argParserState.mateOptionVec = mateOptionVec;
  return 0;
}

static ArgParserConfig argParserDefaultConfig(void) {
  const ArgParserConfig config = {
    .errorOnUnknownArgs = false,
    .valuePointerIsArray = false,
  };
  return config;
}

static ArgParserFlagData argParserConvertFlagDataType(ArgParserFlagData flagData, argParserDataType dataType) {
  // dataType to String function for error logs

  switch (flagData.dataType) {
  default:
  case ARG_PARSER_DATA_TYPE_VOID:
    LogError("Unsupported type conversion: %s -> %s", mateParserTypeToCStr(flagData.dataType), mateParserTypeToCStr(dataType));
    break;
  case ARG_PARSER_DATA_TYPE_INT:
    switch (dataType) {
    default:
    case ARG_PARSER_DATA_TYPE_VOID:
      LogError("Unsupported type conversion: %s -> %s", mateParserTypeToCStr(flagData.dataType), mateParserTypeToCStr(dataType));
      break;
    case ARG_PARSER_DATA_TYPE_INT:
      // already the same data type
      break;
    case ARG_PARSER_DATA_TYPE_FLOAT:
      flagData.data.float64 = (f64)flagData.data.int64;
      flagData.dataType = dataType;
      break;
    case ARG_PARSER_DATA_TYPE_BOOL:
      flagData.dataType = dataType;
      flagData.data.boolean = flagData.data.int64 > 0;
      break;
    case ARG_PARSER_DATA_TYPE_STRING:
      // TODO: this
      flagData.dataType = dataType;
      break;
    }
    break;
  case ARG_PARSER_DATA_TYPE_FLOAT:
    switch (dataType) {
    default:
    case ARG_PARSER_DATA_TYPE_VOID:
      LogError("Unsupported type conversion: %s -> %s", mateParserTypeToCStr(flagData.dataType), mateParserTypeToCStr(dataType));
      break;
    case ARG_PARSER_DATA_TYPE_INT:
      flagData.data.int64 = (i64)flagData.data.float64;
      flagData.dataType = dataType;
      break;
    case ARG_PARSER_DATA_TYPE_FLOAT:
      // already the same data type
      break;
    case ARG_PARSER_DATA_TYPE_BOOL:
      flagData.data.boolean = flagData.data.float64 > 0.0;
      flagData.dataType = dataType;
      break;
    case ARG_PARSER_DATA_TYPE_STRING:
      // TODO:
      flagData.dataType = dataType;
      break;
    }
    break;
  case ARG_PARSER_DATA_TYPE_BOOL:
    switch (dataType) {
    default:
    case ARG_PARSER_DATA_TYPE_VOID:
      LogError("Unsupported type conversion: %s -> %s", mateParserTypeToCStr(flagData.dataType), mateParserTypeToCStr(dataType));
      break;
    case ARG_PARSER_DATA_TYPE_INT:
      flagData.data.int64 = (i64)flagData.data.boolean;
      flagData.dataType = dataType;
      break;
    case ARG_PARSER_DATA_TYPE_FLOAT:
      flagData.data.float64 = (f64)flagData.data.boolean;
      flagData.dataType = dataType;
      break;
    case ARG_PARSER_DATA_TYPE_BOOL:
      // already the same data type
      break;
    case ARG_PARSER_DATA_TYPE_STRING:
      flagData.data.str = flagData.data.boolean ? S("true") : S("false");
      flagData.dataType = dataType;
      break;
    }
    break;
  case ARG_PARSER_DATA_TYPE_STRING:
    switch (dataType) {
    default:
    case ARG_PARSER_DATA_TYPE_VOID:
      LogError("Unsupported type conversion: %s -> %s", mateParserTypeToCStr(flagData.dataType), mateParserTypeToCStr(dataType));
      break;
    case ARG_PARSER_DATA_TYPE_INT: {
      char *endptr = NULL;
      errno = 0;
      i64 val = strtoll(flagData.data.str.data, &endptr, 10);
      if (errno || endptr == flagData.data.str.data || *endptr) val = 0;
      flagData.data.int64 = val;
      flagData.dataType = dataType;
    } break;
    case ARG_PARSER_DATA_TYPE_FLOAT: {
      char *endptr = NULL;
      errno = 0;
      double val = strtod(flagData.data.str.data, &endptr);
      if (errno || !isfinite(val) || endptr == flagData.data.str.data || *endptr) val = 0;
      flagData.data.float64 = val;
      flagData.dataType = dataType;
    } break;
    case ARG_PARSER_DATA_TYPE_BOOL:
      flagData.dataType = dataType;
      StrToLower(flagData.data.str);
      if (StrEq(flagData.data.str, S("true"))) flagData.data.boolean = true;
      else flagData.data.boolean = false; // default to false
      break;
    case ARG_PARSER_DATA_TYPE_STRING:
      // already the same data type
      break;
    }
    break;
  }
  return flagData;
}

void mateDispatchArgumentRules(ArgumentRule *rules, size_t rulesLength) {
  Assert(rules != NULL && rulesLength > 0, "Invalid argument rules!");
  if (mateState.argParserState.config.errorOnUnknownArgs) {
    for (size_t argIdx = 0; argIdx < mateState.argParserState.userOptionVec.length; argIdx++) {
      const ArgParserFlag flag = VecAt(mateState.argParserState.userOptionVec, argIdx);
      bool exists = false;
      for (size_t rulesIdx = 0; rulesIdx < rulesLength; rulesIdx++)
        if (StrEq(rules[rulesIdx].name, flag.name)) exists = true;
      if (!exists) {
        LogError("Invalid option: %s\n", flag.name.data);
        EndBuild();
        exit(1);
      }
    }
  }

  for (size_t rulesIdx = 0; rulesIdx < rulesLength; rulesIdx++) {
    const ArgumentRule rule = rules[rulesIdx];

    // search for an option an the user option vector
    ArgParserFlag *assignedFlag = NULL;
    for (size_t i = 0; i < mateState.argParserState.userOptionVec.length; i++) {
      assignedFlag = &VecAt(mateState.argParserState.userOptionVec, i);
      if (StrEq(assignedFlag->name, rule.name)) break;
      else assignedFlag = NULL;
    }
    // if the option isn't specified by the user, ignore the argument rule
    if (assignedFlag == NULL) continue;

    // assign a datatype
    argParserDataType dataType = ARG_PARSER_DATA_TYPE_VOID;
    if (rule.int32) dataType = ARG_PARSER_DATA_TYPE_INT;
    else if (rule.float32) dataType = ARG_PARSER_DATA_TYPE_FLOAT;
    else if (rule.boolean) dataType = ARG_PARSER_DATA_TYPE_BOOL;
    else if (rule.string) dataType = ARG_PARSER_DATA_TYPE_STRING;
    else if (dataType == ARG_PARSER_DATA_TYPE_VOID) {
      LogWarn("No data type selected! ignoring argument rule");
      continue;
    }

    u32 datatypeSize = 0;
    switch (dataType) {
    case ARG_PARSER_DATA_TYPE_INT:
      datatypeSize = sizeof(i32);
      break;
    case ARG_PARSER_DATA_TYPE_FLOAT:
      datatypeSize = sizeof(f32);
      break;
    case ARG_PARSER_DATA_TYPE_BOOL:
      datatypeSize = sizeof(bool);
      break;
    case ARG_PARSER_DATA_TYPE_STRING:
      datatypeSize = sizeof(String);
      break;
    default:
      LogWarn("Unknown data type! Using size 0");
      datatypeSize = 0;
      break;
    }

    // turn the union vector into a packed array
    u8 *packedValues = calloc(assignedFlag->flagDataVec.length, datatypeSize);
    Assert(packedValues != NULL, "Memory allocation failed!");
    for (size_t i = 0; i < assignedFlag->flagDataVec.length; i++) {
      ArgParserFlagData flagData = VecAt(assignedFlag->flagDataVec, i);
      flagData = argParserConvertFlagDataType(flagData, dataType);
      switch (flagData.dataType) {
      case ARG_PARSER_DATA_TYPE_VOID:
        break;
      case ARG_PARSER_DATA_TYPE_INT: {
        const i64 source = flagData.data.int64;
        int err = memcpy_s(packedValues + (i * datatypeSize), datatypeSize, &source, sizeof(i64));
        Assert(err == 0, "memcpy_s error");
        break;
      }
      case ARG_PARSER_DATA_TYPE_FLOAT: {
        const f64 source = flagData.data.float64;
        int err = memcpy_s(packedValues + (i * datatypeSize), datatypeSize, &source, sizeof(f64));
        Assert(err == 0, "memcpy_s error");
      } break;
      case ARG_PARSER_DATA_TYPE_BOOL: {
        int err = memcpy_s(packedValues + (i * datatypeSize), datatypeSize, &flagData.data.boolean, sizeof(bool));

        Assert(err == 0, "memcpy_s error");
      } break;
      case ARG_PARSER_DATA_TYPE_STRING: {
        int err = memcpy_s(packedValues + (i * datatypeSize), datatypeSize, &flagData.data.str, sizeof(String));
        Assert(err == 0, "memcpy_s error");
      } break;
      }
    }

    // void* and memory and stuff
    if (mateState.argParserState.config.valuePointerIsArray) {
      if (rule.values) *(u8 **)rule.values = packedValues;
    } else {
      if (rule.values) *(u8 **)rule.values = packedValues;
      // if (rule.values) rule.values = *packedValues;
    }
  }
}

static void argParserFreeContext(void) {
  // free user options
  for (size_t i = 0; i < mateState.argParserState.userOptionVec.length; i++) {
    ArgParserFlag flag = VecAt(mateState.argParserState.userOptionVec, i);
    if (flag.flagDataVec.data != NULL && flag.flagDataVec.length > 0) VecFree(flag.flagDataVec);
  }
  if (mateState.argParserState.userOptionVec.data != NULL && mateState.argParserState.userOptionVec.length > 0) VecFree(mateState.argParserState.userOptionVec);

  // free mate options
  for (size_t i = 0; i < mateState.argParserState.mateOptionVec.length; i++) {
    ArgParserFlag flag = VecAt(mateState.argParserState.mateOptionVec, i);
    if (flag.flagDataVec.data != NULL && flag.flagDataVec.length > 0) VecFree(flag.flagDataVec);
  }
  if (mateState.argParserState.mateOptionVec.data != NULL && mateState.argParserState.mateOptionVec.length > 0) VecFree(mateState.argParserState.mateOptionVec);

  // free string arena
  if (mateState.argParserState.stringArena) ArenaFree(mateState.argParserState.stringArena);
}

void StartBuild(int argc, char **argv) {
  LogInit();
  if (!mateState.initConfig) {
    mateSetDefaultState();
  }

  mateState.initConfig = true;
  mateState.startTime = TimeNow();

  Mkdir(mateState.buildDirectory);
  mateReadCache();
  mateRebuild();

  mateState.argParserState.config = argParserDefaultConfig();
  mateParseArguments(argc, argv);
}

static bool mateNeedRebuild(void) {
  File stats = {0};
  errno_t err = FileStats(mateState.mateSource, &stats);
  Assert(err == SUCCESS, "Aborting rebuild: Could not read fileStats for %s, error: %d", mateState.mateSource.data, err);

  if (stats.modifyTime <= mateState.mateCache.lastBuild) {
    return false;
  }

  String mateCachePath = F(mateState.arena, "%s/mate-cache.ini", mateState.buildDirectory.data);
  String modifyTime = F(mateState.arena, FMT_I64, stats.modifyTime);
  IniSet(&mateState.cache, S("modify-time"), modifyTime);

  err = IniWrite(mateCachePath, &mateState.cache);
  Assert(err == SUCCESS, "Aborting rebuild: Could not write cache for path %s, error: %d", mateCachePath.data, err);

  return true;
}

static void mateRebuild(void) {
  if (mateState.mateCache.firstBuild || !mateNeedRebuild()) {
    return;
  }

  String mateExeNew = NormalizeExePath(mateState.arena, F(mateState.arena, "%s/mate-new", mateState.buildDirectory.data));
  String mateExeOld = NormalizeExePath(mateState.arena, F(mateState.arena, "%s/mate-old", mateState.buildDirectory.data));
  String mateExe = NormalizeExePath(mateState.arena, mateState.mateExe);

  String compileCommand;
  if (isMSVC()) {
    compileCommand = F(mateState.arena, "cl.exe \"%s\" /Fe:\"%s\"", mateState.mateSource.data, mateExeNew.data);
  } else {
    compileCommand = F(mateState.arena, "%s \"%s\" -o \"%s\"", CompilerToStr(mateState.compiler).data, mateState.mateSource.data, mateExeNew.data);
  }

  LogWarn("%s changed rebuilding...", mateState.mateSource.data);
  errno_t rebuildErr = RunCommand(compileCommand);
  Assert(rebuildErr == SUCCESS, "MateRebuild: failed command %s, err: %d", compileCommand.data, rebuildErr);

  errno_t renameErr = FileRename(mateExe, mateExeOld);
  Assert(renameErr == SUCCESS, "MateRebuild: failed renaming original executable failed, err: %d", renameErr);

  renameErr = FileRename(mateExeNew, mateExe);
  Assert(renameErr == SUCCESS, "MateRebuild: failed renaming new executable into old: %d", renameErr);

  LogInfo("Rebuild finished, running %s", mateExe.data);
  errno_t err = RunCommand(mateExe);
  exit(err);
}

static bool mateIsValidExecutable(String *exePath) {
  if (exePath->data[0] == '.') {
    return false;
  }

  for (size_t i = 0; i < exePath->length; i++) {
    char currChar = exePath->data[i];
    if (currChar == '/' || currChar == '\\') {
      return false;
    }
  }
  return true;
}

static StaticLib mateDefaultStaticLib(void) {
  StaticLib result = {0};
  result.output = S("");
  result.flags = S("");
  result.arFlags = S("rcs");
  return result;
}

static StaticLib mateParseStaticLibOptions(StaticLibOptions *options) {
  StaticLib result = {0};
  result.output = StrNew(mateState.arena, options->output);
  Assert(!StrIsNull(result.output),
         "MateParseStaticLibOptions: failed, StaticLibOptions.output should never be null, please define the output name like this: \n"
         "\n"
         "CreateStaticLib((StaticLibOptions) { .output = \"libexample\"});");

  // NOTE: For custom build folder look into `CreateConfig`, e.g:
  // CreateConfig((MateOptions){ .buildDirectory = "./custom-dir" });
  Assert(mateIsValidExecutable(&result.output),
         "MateParseStaticLibOptions: failed, StaticLibOptions.output shouldn't be a path, e.g: \n"
         "\n"
         "Correct:\n"
         "CreateStaticLib((StaticLibOptions) { .output = \"libexample\"});\n"
         "\n"
         "Incorrect:\n"
         "CreateStaticLib((StaticLibOptions) { .output = \"./output/libexample.a\"});");

  result.flags = StrNew(mateState.arena, options->flags);
  result.arFlags = StrNew(mateState.arena, options->arFlags);
  return result;
}

StaticLib CreateStaticLib(StaticLibOptions staticLibOptions) {
  Assert(!isMSVC(), "CreateStaticLib: MSVC compiler not yet implemented for static libraries");
  Assert(mateState.initConfig,
         "CreateStaticLib: before creating a static library you must use StartBuild(), like this: \n"
         "\n"
         "StartBuild()\n"
         "{\n"
         " // ...\n"
         "}\n"
         "EndBuild()");

  StaticLib result = mateDefaultStaticLib();
  StaticLib options = mateParseStaticLibOptions(&staticLibOptions);

  String staticLibOutput = NormalizeStaticLibPath(mateState.arena, options.output);
  result.output = NormalizePath(mateState.arena, staticLibOutput);

  FlagBuilder flagBuilder = FlagBuilderCreate();
  if (!StrIsNull(options.flags)) {
    FlagBuilderAddString(&flagBuilder, &options.flags);
  }

  if (mateState.compiler == GCC) {
    switch (staticLibOptions.error) {
    case FLAG_ERROR:
      FlagBuilderAdd(&flagBuilder, "fdiagnostics-color=always");
      break;
    case FLAG_ERROR_MAX:
      FlagBuilderAdd(&flagBuilder, "fdiagnostics-color=always", "fdiagnostics-show-caret", "fdiagnostics-show-option", "fdiagnostics-generate-patch");
      break;
    }
  } else if (mateState.compiler == CLANG) {
    switch (staticLibOptions.error) {
    case FLAG_ERROR:
      FlagBuilderAdd(&flagBuilder, "fcolor-diagnostics");
      break;
    case FLAG_ERROR_MAX:
      FlagBuilderAdd(&flagBuilder, "fcolor-diagnostics", "fcaret-diagnostics", "fdiagnostics-fixit-info", "fdiagnostics-show-option");
      break;
    }
  } else if (mateState.compiler == MSVC) {
    switch (staticLibOptions.error) {
    case FLAG_ERROR:
      FlagBuilderAdd(&flagBuilder, "nologo");
      break;
    case FLAG_ERROR_MAX:
      FlagBuilderAdd(&flagBuilder, "nologo", "diagnostics:caret");
      break;
    }
  }

  if (staticLibOptions.warnings != 0) {
    if (mateState.compiler == MSVC) {
      switch (staticLibOptions.warnings) {
      case FLAG_WARNINGS_NONE:
        FlagBuilderAdd(&flagBuilder, "W0");
        break;
      case FLAG_WARNINGS_MINIMAL:
        FlagBuilderAdd(&flagBuilder, "W3");
        break;
      case FLAG_WARNINGS:
        FlagBuilderAdd(&flagBuilder, "W4");
        break;
      case FLAG_WARNINGS_VERBOSE:
        FlagBuilderAdd(&flagBuilder, "Wall");
        break;
      }
    } else {
      switch (staticLibOptions.warnings) {
      case FLAG_WARNINGS_NONE:
        FlagBuilderAdd(&flagBuilder, "w");
        break;
      case FLAG_WARNINGS_MINIMAL:
        FlagBuilderAdd(&flagBuilder, "Wall");
        break;
      case FLAG_WARNINGS:
        FlagBuilderAdd(&flagBuilder, "Wall", "Wextra");
        break;
      case FLAG_WARNINGS_VERBOSE:
        FlagBuilderAdd(&flagBuilder, "Wall", "Wextra", "Wpedantic");
        break;
      }
    }
  }

  if (staticLibOptions.debug != 0) {
    if (mateState.compiler == MSVC) {
      switch (staticLibOptions.debug) {
      case FLAG_DEBUG_MINIMAL:
        FlagBuilderAdd(&flagBuilder, "Zi");
        break;
      case FLAG_DEBUG_MEDIUM:
      case FLAG_DEBUG:
        FlagBuilderAdd(&flagBuilder, "ZI");
        break;
      }
    } else {
      switch (staticLibOptions.debug) {
      case FLAG_DEBUG_MINIMAL:
        FlagBuilderAdd(&flagBuilder, "g1");
        break;
      case FLAG_DEBUG_MEDIUM:
        FlagBuilderAdd(&flagBuilder, "g2");
        break;
      case FLAG_DEBUG:
        FlagBuilderAdd(&flagBuilder, "g3");
        break;
      }
    }
  }

  if (staticLibOptions.optimization != 0) {
    if (mateState.compiler == MSVC) {
      switch (staticLibOptions.optimization) {
      case FLAG_OPTIMIZATION_NONE:
        FlagBuilderAdd(&flagBuilder, "Od");
        break;
      case FLAG_OPTIMIZATION_BASIC:
        FlagBuilderAdd(&flagBuilder, "O1");
        break;
      case FLAG_OPTIMIZATION:
        FlagBuilderAdd(&flagBuilder, "O2");
        break;
      case FLAG_OPTIMIZATION_SIZE:
        FlagBuilderAdd(&flagBuilder, "O1");
        break;
      case FLAG_OPTIMIZATION_AGGRESSIVE:
        FlagBuilderAdd(&flagBuilder, "Ox");
        break;
      }
    } else {
      switch (staticLibOptions.optimization) {
      case FLAG_OPTIMIZATION_NONE:
        FlagBuilderAdd(&flagBuilder, "O0");
        break;
      case FLAG_OPTIMIZATION_BASIC:
        FlagBuilderAdd(&flagBuilder, "O1");
        break;
      case FLAG_OPTIMIZATION:
        FlagBuilderAdd(&flagBuilder, "O2");
        break;
      case FLAG_OPTIMIZATION_SIZE:
        FlagBuilderAdd(&flagBuilder, "Os");
        break;
      case FLAG_OPTIMIZATION_AGGRESSIVE:
        FlagBuilderAdd(&flagBuilder, "O3");
        break;
      }
    }
  }

  if (staticLibOptions.std != 0) {
    if (mateState.compiler == MSVC) {
      switch (staticLibOptions.std) {
      case FLAG_STD_C99:
      case FLAG_STD_C11:
        FlagBuilderAdd(&flagBuilder, "std:c11");
        break;
      case FLAG_STD_C17:
        FlagBuilderAdd(&flagBuilder, "std:c17");
        break;
      case FLAG_STD_C23:
      case FLAG_STD_C2X:
        FlagBuilderAdd(&flagBuilder, "std:clatest"); // NOTE: MSVC doesn't have C23 yet
        break;
      }
    } else {
      switch (staticLibOptions.std) {
      case FLAG_STD_C99:
        FlagBuilderAdd(&flagBuilder, "std=c99");
        break;
      case FLAG_STD_C11:
        FlagBuilderAdd(&flagBuilder, "std=c11");
        break;
      case FLAG_STD_C17:
        FlagBuilderAdd(&flagBuilder, "std=c17");
        break;
      case FLAG_STD_C23:
        FlagBuilderAdd(&flagBuilder, "std=c2x");
        break;
      case FLAG_STD_C2X:
        FlagBuilderAdd(&flagBuilder, "std=c2x");
        break;
      }
    }
  }

  if (!StrIsNull(flagBuilder.buffer)) {
    result.flags = flagBuilder.buffer;
  }

  if (!StrIsNull(options.arFlags)) {
    result.arFlags = options.arFlags;
  }

  String optionIncludes = s(staticLibOptions.includes);
  if (!StrIsNull(optionIncludes)) {
    result.includes = optionIncludes;
  }

  String optionLibs = s(staticLibOptions.libs);
  if (!StrIsNull(optionLibs)) {
    result.libs = optionLibs;
  }

  result.ninjaBuildPath = F(mateState.arena, "%s/static-%s.ninja", mateState.buildDirectory.data, NormalizeExtension(mateState.arena, result.output).data);
  return result;
}

static Executable mateDefaultExecutable(void) {
  Executable result = {0};
  String executableOutput = NormalizeExePath(mateState.arena, S("main"));
  result.output = NormalizePath(mateState.arena, executableOutput);
  result.linkerFlags = S("");
  result.flags = S("");
  return result;
}

static Executable mateParseExecutableOptions(ExecutableOptions *options) {
  Executable result = {0};
  result.output = StrNew(mateState.arena, options->output);
  result.flags = StrNew(mateState.arena, options->flags);
  result.linkerFlags = StrNew(mateState.arena, options->linkerFlags);
  return result;
}

Executable CreateExecutable(ExecutableOptions executableOptions) {
  Assert(mateState.initConfig,
         "CreateExecutable: before creating an executable you must use StartBuild(), like this: \n"
         "\n"
         "StartBuild()\n"
         "{\n"
         " // ...\n"
         "}\n"
         "EndBuild()");

  Executable result = mateDefaultExecutable();
  Executable options = mateParseExecutableOptions(&executableOptions);
  if (!StrIsNull(options.output)) {
    String executableOutput = NormalizeExePath(mateState.arena, options.output);
    // NOTE: For custom build folder look into `CreateConfig`, e.g:
    // CreateConfig((MateOptions){ .buildDirectory = "./custom-dir" });
    Assert(mateIsValidExecutable(&executableOutput),
           "MateParseExecutable: failed, ExecutableOptions.output shouldn't be a path, e.g: \n"
           "\n"
           "Correct:\n"
           "CreateExecutable((ExecutableOptions) { .output = \"main\"});\n"
           "\n"
           "Incorrect:\n"
           "CreateExecutable((ExecutableOptions) { .output = \"./output/main\"});");
    result.output = NormalizePath(mateState.arena, executableOutput);
  }

  FlagBuilder flagBuilder = FlagBuilderCreate();
  if (!StrIsNull(options.flags)) {
    FlagBuilderAddString(&flagBuilder, &options.flags);
  }

  if (mateState.compiler == GCC) {
    switch (executableOptions.error) {
    case FLAG_ERROR:
      FlagBuilderAdd(&flagBuilder, "fdiagnostics-color=always");
      break;
    case FLAG_ERROR_MAX:
      FlagBuilderAdd(&flagBuilder, "fdiagnostics-color=always", "fdiagnostics-show-caret", "fdiagnostics-show-option", "fdiagnostics-generate-patch");
      break;
    }
  } else if (mateState.compiler == CLANG) {
    switch (executableOptions.error) {
    case FLAG_ERROR:
      FlagBuilderAdd(&flagBuilder, "fcolor-diagnostics");
      break;
    case FLAG_ERROR_MAX:
      FlagBuilderAdd(&flagBuilder, "fcolor-diagnostics", "fcaret-diagnostics", "fdiagnostics-fixit-info", "fdiagnostics-show-option");
      break;
    }
  } else if (mateState.compiler == MSVC) {
    switch (executableOptions.error) {
    case FLAG_ERROR:
      FlagBuilderAdd(&flagBuilder, "nologo");
      break;
    case FLAG_ERROR_MAX:
      FlagBuilderAdd(&flagBuilder, "nologo", "diagnostics:caret");
      break;
    }
  }

  if (executableOptions.warnings != 0) {
    if (mateState.compiler == MSVC) {
      switch (executableOptions.warnings) {
      case FLAG_WARNINGS_NONE:
        FlagBuilderAdd(&flagBuilder, "W0");
        break;
      case FLAG_WARNINGS_MINIMAL:
        FlagBuilderAdd(&flagBuilder, "W3");
        break;
      case FLAG_WARNINGS:
        FlagBuilderAdd(&flagBuilder, "W4");
        break;
      case FLAG_WARNINGS_VERBOSE:
        FlagBuilderAdd(&flagBuilder, "Wall");
        break;
      }
    } else {
      switch (executableOptions.warnings) {
      case FLAG_WARNINGS_NONE:
        FlagBuilderAdd(&flagBuilder, "w");
        break;
      case FLAG_WARNINGS_MINIMAL:
        FlagBuilderAdd(&flagBuilder, "Wall");
        break;
      case FLAG_WARNINGS:
        FlagBuilderAdd(&flagBuilder, "Wall", "Wextra");
        break;
      case FLAG_WARNINGS_VERBOSE:
        FlagBuilderAdd(&flagBuilder, "Wall", "Wextra", "Wpedantic");
        break;
      }
    }
  }

  if (executableOptions.debug != 0) {
    if (mateState.compiler == MSVC) {
      switch (executableOptions.debug) {
      case FLAG_DEBUG_MINIMAL:
        FlagBuilderAdd(&flagBuilder, "Zi");
        break;
      case FLAG_DEBUG_MEDIUM:
      case FLAG_DEBUG:
        FlagBuilderAdd(&flagBuilder, "ZI");
        break;
      }
    } else {
      switch (executableOptions.debug) {
      case FLAG_DEBUG_MINIMAL:
        FlagBuilderAdd(&flagBuilder, "g1");
        break;
      case FLAG_DEBUG_MEDIUM:
        FlagBuilderAdd(&flagBuilder, "g2");
        break;
      case FLAG_DEBUG:
        FlagBuilderAdd(&flagBuilder, "g3");
        break;
      }
    }
  }

  if (executableOptions.optimization != 0) {
    if (mateState.compiler == MSVC) {
      switch (executableOptions.optimization) {
      case FLAG_OPTIMIZATION_NONE:
        FlagBuilderAdd(&flagBuilder, "Od");
        break;
      case FLAG_OPTIMIZATION_BASIC:
        FlagBuilderAdd(&flagBuilder, "O1");
        break;
      case FLAG_OPTIMIZATION:
        FlagBuilderAdd(&flagBuilder, "O2");
        break;
      case FLAG_OPTIMIZATION_SIZE:
        FlagBuilderAdd(&flagBuilder, "O1");
        break;
      case FLAG_OPTIMIZATION_AGGRESSIVE:
        FlagBuilderAdd(&flagBuilder, "Ox");
        break;
      }
    } else {
      switch (executableOptions.optimization) {
      case FLAG_OPTIMIZATION_NONE:
        FlagBuilderAdd(&flagBuilder, "O0");
        break;
      case FLAG_OPTIMIZATION_BASIC:
        FlagBuilderAdd(&flagBuilder, "O1");
        break;
      case FLAG_OPTIMIZATION:
        FlagBuilderAdd(&flagBuilder, "O2");
        break;
      case FLAG_OPTIMIZATION_SIZE:
        FlagBuilderAdd(&flagBuilder, "Os");
        break;
      case FLAG_OPTIMIZATION_AGGRESSIVE:
        FlagBuilderAdd(&flagBuilder, "O3");
        break;
      }
    }
  }

  if (executableOptions.std != 0) {
    if (mateState.compiler == MSVC) {
      switch (executableOptions.std) {
      case FLAG_STD_C99:
      case FLAG_STD_C11:
        FlagBuilderAdd(&flagBuilder, "std:c11");
        break;
      case FLAG_STD_C17:
        FlagBuilderAdd(&flagBuilder, "std:c17");
        break;
      case FLAG_STD_C23:
      case FLAG_STD_C2X:
        // NOTE: MSVC doesn't have C23 yet
        FlagBuilderAdd(&flagBuilder, "std:clatest");
        break;
      }
    } else {
      switch (executableOptions.std) {
      case FLAG_STD_C99:
        FlagBuilderAdd(&flagBuilder, "std=c99");
        break;
      case FLAG_STD_C11:
        FlagBuilderAdd(&flagBuilder, "std=c11");
        break;
      case FLAG_STD_C17:
        FlagBuilderAdd(&flagBuilder, "std=c17");
        break;
      case FLAG_STD_C23:
        FlagBuilderAdd(&flagBuilder, "std=c2x");
        break;
      case FLAG_STD_C2X:
        FlagBuilderAdd(&flagBuilder, "std=c2x");
        break;
      }
    }
  }

  if (!StrIsNull(flagBuilder.buffer)) {
    result.flags = flagBuilder.buffer;
  }

  if (!StrIsNull(options.linkerFlags)) {
    result.linkerFlags = options.linkerFlags;
  }

  String optionIncludes = s(executableOptions.includes);
  if (!StrIsNull(optionIncludes)) {
    result.includes = optionIncludes;
  }

  String optionLibs = s(executableOptions.libs);
  if (!StrIsNull(optionLibs)) {
    result.libs = optionLibs;
  }

  result.ninjaBuildPath = F(mateState.arena, "%s/exe-%s.ninja", mateState.buildDirectory.data, NormalizeExtension(mateState.arena, result.output).data);
  return result;
}

static CreateCompileCommandsError mateCreateCompileCommands(String *ninjaBuildPath) {
  FILE *ninjaPipe;
  FILE *outputFile;
  char buffer[4096];
  size_t bytes_read;

  String compileCommandsPath = NormalizePath(mateState.arena, F(mateState.arena, "%s/compile_commands.json", mateState.buildDirectory.data));
  errno_t err = fopen_s(&outputFile, compileCommandsPath.data, "w");
  if (err != SUCCESS || outputFile == NULL) {
    LogError("CreateCompileCommands: Failed to open file %s, err: %d", compileCommandsPath.data, err);
    return COMPILE_COMMANDS_FAILED_OPEN_FILE;
  }

  String compdbCommand;
  if (mateState.mateCache.samuraiBuild == true) {
    String samuraiOutputPath = F(mateState.arena, "%s/samurai", mateState.buildDirectory.data);
    compdbCommand = NormalizePath(mateState.arena, F(mateState.arena, "%s -f %s -t compdb compile", samuraiOutputPath.data, ninjaBuildPath->data));
  }

  if (mateState.mateCache.samuraiBuild == false) {
    compdbCommand = NormalizePath(mateState.arena, F(mateState.arena, "ninja -f %s -t compdb compile", ninjaBuildPath->data));
  }

  ninjaPipe = popen(compdbCommand.data, "r");
  if (ninjaPipe == NULL) {
    LogError("CreateCompileCommands: Failed to run compdb command, %s", compdbCommand.data);
    fclose(outputFile);
    return COMPILE_COMMANDS_FAILED_COMPDB;
  }

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), ninjaPipe)) > 0) {
    fwrite(buffer, 1, bytes_read, outputFile);
  }

  fclose(outputFile);
  errno_t status = pclose(ninjaPipe);
  if (status != SUCCESS) {
    LogError("CreateCompileCommands: Command failed with status %d\n", status);
    return COMPILE_COMMANDS_FAILED_COMPDB;
  }

  LogSuccess("Successfully created %s", NormalizePathEnd(mateState.arena, compileCommandsPath).data);
  return COMPILE_COMMANDS_SUCCESS;
}

static void mateAddFiles(StringVector *sources, char **files, size_t size) {
  for (size_t i = 0; i < size; i++) {
    String currFile = s(files[i]);
    mateAddFile(sources, currFile);
  }
}

static void mateAddFile(StringVector *sources, String source) {
  bool isGlob = false;
  for (size_t i = 0; i < source.length; i++) {
    if (source.data[i] == '*') {
      isGlob = true;
      break;
    }
  }

  Assert(source.length > 2 && source.data[0] == '.' && source.data[1] == '/',
         "AddFile: failed to a add file, to add a file it has to "
         "contain the relative path, for example AddFile(\"./main.c\")");

  Assert(source.data[source.length - 1] != '/',
         "AddFile: failed to add a file, you can't add a slash at the end of a path.\n"
         "For example, valid: AddFile(\"./main.c\"), invalid: AddFile(\"./main.c/\")");

  if (!isGlob) {
    VecPush((*sources), source);
    return;
  }

  String directory = {0};
  i32 lastSlash = -1;
  for (size_t i = 0; i < source.length; i++) {
    if (source.data[i] == '/') {
      lastSlash = i;
    }
  }

  directory = StrSlice(mateState.arena, source, 0, lastSlash);
  String pattern = StrSlice(mateState.arena, source, lastSlash + 1, source.length);

  StringVector files = ListDir(mateState.arena, directory);
  for (size_t i = 0; i < files.length; i++) {
    String file = VecAt(files, i);

    if (mateGlobMatch(pattern, file)) {
      String finalSource = F(mateState.arena, "%s/%s", directory.data, file.data);
      VecPush((*sources), finalSource);
    }
  }
}

static bool mateRemoveFile(StringVector *sources, String source) {
  Assert(sources->length > 0, "RemoveFile: Before removing a file you must first add a file, use: AddFile()");

  for (size_t i = 0; i < sources->length; i++) {
    String *currValue = VecAtPtr((*sources), i);
    if (StrEq(source, *currValue)) {
      currValue->data = NULL;
      currValue->length = 0;
      return true;
    }
  }
  return false;
}

static void mateInstallExecutable(Executable *executable) {
  Assert(executable->sources.length != 0, "InstallExecutable: Executable has zero sources, add at least one with AddFile(\"./main.c\")");
  Assert(!StrIsNull(executable->output), "InstallExecutable: Before installing executable you must first CreateExecutable()");

  StringBuilder builder = StringBuilderReserve(mateState.arena, 1024);

  // Compiler
  StringBuilderAppend(mateState.arena, &builder, &S("cc = "));
  String compiler = CompilerToStr(mateState.compiler);
  StringBuilderAppend(mateState.arena, &builder, &compiler);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Linker flags
  if (executable->linkerFlags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("linker_flags = "));
    StringBuilderAppend(mateState.arena, &builder, &executable->linkerFlags);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Compiler flags
  if (executable->flags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("flags = "));
    StringBuilderAppend(mateState.arena, &builder, &executable->flags);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Include paths
  if (executable->includes.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("includes = "));
    StringBuilderAppend(mateState.arena, &builder, &executable->includes);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Libraries
  if (executable->libs.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("libs = "));
    StringBuilderAppend(mateState.arena, &builder, &executable->libs);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Current working directory
  String cwd_path = mateConvertNinjaPath(s(GetCwd()));
  StringBuilderAppend(mateState.arena, &builder, &S("cwd = "));
  StringBuilderAppend(mateState.arena, &builder, &cwd_path);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Build directory
  String build_dir_path = mateConvertNinjaPath(mateState.buildDirectory);
  StringBuilderAppend(mateState.arena, &builder, &S("builddir = "));
  StringBuilderAppend(mateState.arena, &builder, &build_dir_path);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Target
  StringBuilderAppend(mateState.arena, &builder, &S("target = $builddir/"));
  StringBuilderAppend(mateState.arena, &builder, &executable->output);
  StringBuilderAppend(mateState.arena, &builder, &S("\n\n"));

  // Link command
  StringBuilderAppend(mateState.arena, &builder, &S("rule link\n  command = $cc"));
  if (executable->flags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $flags"));
  }

  if (executable->linkerFlags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $linker_flags"));
  }

  if (isMSVC()) {
    StringBuilderAppend(mateState.arena, &builder, &S(" /Fe:$out $in"));
  } else {
    StringBuilderAppend(mateState.arena, &builder, &S(" -o $out $in"));
  }

  if (executable->libs.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $libs"));
  }
  StringBuilderAppend(mateState.arena, &builder, &S("\n\n"));

  // Compile command
  StringBuilderAppend(mateState.arena, &builder, &S("rule compile\n  command = $cc"));
  if (executable->flags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $flags"));
  }
  if (executable->includes.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $includes"));
  }

  if (isMSVC()) {
    StringBuilderAppend(mateState.arena, &builder, &S(" /c $in /Fo:$out\n\n"));
  } else {
    StringBuilderAppend(mateState.arena, &builder, &S(" -c $in -o $out\n\n"));
  }

  // Build individual source files
  StringVector outputFiles = mateOutputTransformer(executable->sources);
  StringBuilder outputBuilder = StringBuilderCreate(mateState.arena);
  for (size_t i = 0; i < executable->sources.length; i++) {
    String currSource = VecAt(executable->sources, i);
    if (StrIsNull(currSource)) continue;

    String outputFile = VecAt(outputFiles, i);
    String sourceFile = NormalizePathStart(mateState.arena, currSource);

    // Source build command
    StringBuilderAppend(mateState.arena, &builder, &S("build $builddir/"));
    StringBuilderAppend(mateState.arena, &builder, &outputFile);
    StringBuilderAppend(mateState.arena, &builder, &S(": compile $cwd/"));
    StringBuilderAppend(mateState.arena, &builder, &sourceFile);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));

    // Add to output files list
    if (outputBuilder.buffer.length == 0) {
      StringBuilderAppend(mateState.arena, &outputBuilder, &S("$builddir/"));
      StringBuilderAppend(mateState.arena, &outputBuilder, &outputFile);
    } else {
      StringBuilderAppend(mateState.arena, &outputBuilder, &S(" $builddir/"));
      StringBuilderAppend(mateState.arena, &outputBuilder, &outputFile);
    }
  }

  // Build target
  StringBuilderAppend(mateState.arena, &builder, &S("build $target: link "));
  StringBuilderAppend(mateState.arena, &builder, &outputBuilder.buffer);
  StringBuilderAppend(mateState.arena, &builder, &S("\n\n"));

  // Default target
  StringBuilderAppend(mateState.arena, &builder, &S("default $target\n"));

  String ninjaBuildPath = executable->ninjaBuildPath;
  errno_t errWrite = FileWrite(ninjaBuildPath, builder.buffer);
  Assert(errWrite == SUCCESS, "InstallExecutable: failed to write build.ninja for %s, err: %d", ninjaBuildPath.data, errWrite);

  String buildCommand;
  if (mateState.mateCache.samuraiBuild) {
    String samuraiOutputPath = F(mateState.arena, "%s/samurai", mateState.buildDirectory.data);
    buildCommand = F(mateState.arena, "%s -f %s", samuraiOutputPath.data, ninjaBuildPath.data);
  } else {
    buildCommand = F(mateState.arena, "ninja -f %s", ninjaBuildPath.data);
  }

  i64 err = RunCommand(buildCommand);
  Assert(err == SUCCESS, "InstallExecutable: Ninja file compilation failed with code: " FMT_I64, err);

  LogSuccess("Ninja file compilation done for %s", NormalizePathEnd(mateState.arena, ninjaBuildPath).data);
  mateState.totalTime = TimeNow() - mateState.startTime;

#if defined(PLATFORM_WIN)
  executable->outputPath = F(mateState.arena, "%s\\%s", mateState.buildDirectory.data, executable->output.data);
#else
  executable->outputPath = F(mateState.arena, "%s/%s", mateState.buildDirectory.data, executable->output.data);
#endif
}

static void mateInstallStaticLib(StaticLib *staticLib) {
  Assert(staticLib->sources.length != 0, "InstallStaticLib: Static Library has zero sources, add at least one with AddFile(\"./main.c\")");
  Assert(!StrIsNull(staticLib->output), "InstallStaticLib: Before installing static library you must first CreateStaticLib()");

  StringBuilder builder = StringBuilderReserve(mateState.arena, 1024);

  // Compiler
  StringBuilderAppend(mateState.arena, &builder, &S("cc = "));
  String compiler = CompilerToStr(mateState.compiler);
  StringBuilderAppend(mateState.arena, &builder, &compiler);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Archive
  StringBuilderAppend(mateState.arena, &builder, &S("ar = ar\n")); // TODO: Add different ar for MSVC

  // Compiler flags
  if (staticLib->flags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("flags = "));
    StringBuilderAppend(mateState.arena, &builder, &staticLib->flags);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Archive flags
  if (staticLib->arFlags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("ar_flags = "));
    StringBuilderAppend(mateState.arena, &builder, &staticLib->arFlags);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Include paths
  if (staticLib->includes.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("includes = "));
    StringBuilderAppend(mateState.arena, &builder, &staticLib->includes);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Current working directory
  String cwd_path = mateConvertNinjaPath(s(GetCwd()));
  StringBuilderAppend(mateState.arena, &builder, &S("cwd = "));
  StringBuilderAppend(mateState.arena, &builder, &cwd_path);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Build directory
  String build_dir_path = mateConvertNinjaPath(mateState.buildDirectory);
  StringBuilderAppend(mateState.arena, &builder, &S("builddir = "));
  StringBuilderAppend(mateState.arena, &builder, &build_dir_path);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Target
  StringBuilderAppend(mateState.arena, &builder, &S("target = $builddir/"));
  StringBuilderAppend(mateState.arena, &builder, &staticLib->output);
  StringBuilderAppend(mateState.arena, &builder, &S("\n\n"));

  // Archive command
  StringBuilderAppend(mateState.arena, &builder, &S("rule archive\n  command = $ar $ar_flags $out $in\n\n"));

  // Compile command
  StringBuilderAppend(mateState.arena, &builder, &S("rule compile\n  command = $cc"));
  if (staticLib->flags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $flags"));
  }
  if (staticLib->includes.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $includes"));
  }
  StringBuilderAppend(mateState.arena, &builder, &S(" -c $in -o $out\n\n"));

  // Build individual source files
  StringVector outputFiles = mateOutputTransformer(staticLib->sources);
  StringBuilder outputBuilder = StringBuilderCreate(mateState.arena);
  for (size_t i = 0; i < staticLib->sources.length; i++) {
    String currSource = VecAt(staticLib->sources, i);
    if (StrIsNull(currSource)) continue;

    String outputFile = VecAt(outputFiles, i);
    String sourceFile = NormalizePathStart(mateState.arena, currSource);

    // Source build command
    StringBuilderAppend(mateState.arena, &builder, &S("build $builddir/"));
    StringBuilderAppend(mateState.arena, &builder, &outputFile);
    StringBuilderAppend(mateState.arena, &builder, &S(": compile $cwd/"));
    StringBuilderAppend(mateState.arena, &builder, &sourceFile);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));

    // Add to output files list
    if (outputBuilder.buffer.length == 0) {
      StringBuilderAppend(mateState.arena, &outputBuilder, &S("$builddir/"));
      StringBuilderAppend(mateState.arena, &outputBuilder, &outputFile);
    } else {
      StringBuilderAppend(mateState.arena, &outputBuilder, &S(" $builddir/"));
      StringBuilderAppend(mateState.arena, &outputBuilder, &outputFile);
    }
  }

  // Build target
  StringBuilderAppend(mateState.arena, &builder, &S("build $target: archive "));
  StringBuilderAppend(mateState.arena, &builder, &outputBuilder.buffer);
  StringBuilderAppend(mateState.arena, &builder, &S("\n\n"));

  // Default target
  StringBuilderAppend(mateState.arena, &builder, &S("default $target\n"));

  String ninjaBuildPath = staticLib->ninjaBuildPath;
  errno_t errWrite = FileWrite(ninjaBuildPath, builder.buffer);
  Assert(errWrite == SUCCESS, "InstallStaticLib: failed to write build-static-library.ninja for %s, err: %d", ninjaBuildPath.data, errWrite);

  String buildCommand;
  if (mateState.mateCache.samuraiBuild) {
    String samuraiOutputPath = F(mateState.arena, "%s/samurai", mateState.buildDirectory.data);
    buildCommand = F(mateState.arena, "%s -f %s", samuraiOutputPath.data, ninjaBuildPath.data);
  } else {
    buildCommand = F(mateState.arena, "ninja -f %s", ninjaBuildPath.data);
  }

  i64 err = RunCommand(buildCommand);
  Assert(err == SUCCESS, "InstallStaticLib: Ninja file compilation failed with code: " FMT_I64, err);

  LogSuccess("Ninja file compilation done for %s", NormalizePathEnd(mateState.arena, ninjaBuildPath).data);
  mateState.totalTime = TimeNow() - mateState.startTime;

#if defined(PLATFORM_WIN)
  staticLib->outputPath = F(mateState.arena, "%s\\%s", mateState.buildDirectory.data, staticLib->output.data);
#else
  staticLib->outputPath = F(mateState.arena, "%s/%s", mateState.buildDirectory.data, staticLib->output.data);
#endif
}

static void mateAddLibraryPaths(String *targetLibs, StringVector *libs) {
  StringBuilder builder = StringBuilderCreate(mateState.arena);

  if (isMSVC() && targetLibs->length == 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("/link"));
  }

  if (targetLibs->length) {
    StringBuilderAppend(mateState.arena, &builder, targetLibs);
  }

  if (isMSVC()) {
    // MSVC format: /LIBPATH:"path"
    for (size_t i = 0; i < libs->length; i++) {
      String currLib = VecAt((*libs), i);
      String buffer = F(mateState.arena, " /LIBPATH:\"%s\"", currLib.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  } else {
    // GCC/Clang format: -L"path"
    for (size_t i = 0; i < libs->length; i++) {
      String currLib = VecAt((*libs), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mateState.arena, "-L\"%s\"", currLib.data);
        StringBuilderAppend(mateState.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(mateState.arena, " -L\"%s\"", currLib.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  }

  *targetLibs = builder.buffer;
}

static void mateLinkSystemLibraries(String *targetLibs, StringVector *libs) {
  StringBuilder builder = StringBuilderCreate(mateState.arena);

  if (isMSVC() && targetLibs->length == 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("/link"));
  }

  if (targetLibs->length) {
    StringBuilderAppend(mateState.arena, &builder, targetLibs);
  }

  if (isMSVC()) {
    // MSVC format: library.lib
    for (size_t i = 0; i < libs->length; i++) {
      String currLib = VecAt((*libs), i);
      String buffer = F(mateState.arena, " %s.lib", currLib.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  } else {
    // GCC/Clang format: -llib
    for (size_t i = 0; i < libs->length; i++) {
      String currLib = VecAt((*libs), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mateState.arena, "-l%s", currLib.data);
        StringBuilderAppend(mateState.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(mateState.arena, " -l%s", currLib.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  }

  *targetLibs = builder.buffer;
}

static void mateLinkFrameworks(String *targetLibs, StringVector *frameworks) {
  mateLinkFrameworksWithOptions(targetLibs, none, frameworks);
}

static void mateLinkFrameworksWithOptions(String *targetLibs, LinkFrameworkOptions options, StringVector *frameworks) {
  Assert(!isGCC(),
         "LinkFrameworks: Automatic framework linking is not supported by GCC. "
         "Use standard linking functions after adding a framework path instead.");
  Assert(isClang(), "LinkFrameworks: This function is only supported for Clang.");

  char *frameworkFlag = NULL;
  switch (options) {
  case none:
    frameworkFlag = "-framework";
    break;
  case needed:
    frameworkFlag = "-needed_framework";
    break;
  case weak:
    frameworkFlag = "-weak_framework";
    break;
  default:
    Assert(0, "LinkFrameworks: Invalid framework linking option provided.");
  }

  StringBuilder builder = StringBuilderCreate(mateState.arena);

  if (targetLibs->length) {
    StringBuilderAppend(mateState.arena, &builder, targetLibs);
  }

  for (size_t i = 0; i < frameworks->length; i++) {
    String currFW = VecAt((*frameworks), i);
    if (i == 0 && builder.buffer.length == 0) {
      String buffer = F(mateState.arena, "%s %s", frameworkFlag, currFW.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
      continue;
    }
    String buffer = F(mateState.arena, " %s %s", frameworkFlag, currFW.data);
    StringBuilderAppend(mateState.arena, &builder, &buffer);
  }

  *targetLibs = builder.buffer;
}

static void mateAddIncludePaths(String *targetIncludes, StringVector *includes) {
  StringBuilder builder = StringBuilderCreate(mateState.arena);

  if (targetIncludes->length) {
    StringBuilderAppend(mateState.arena, &builder, targetIncludes);
    StringBuilderAppend(mateState.arena, &builder, &S(" "));
  }

  if (isMSVC()) {
    // MSVC format: /I"path"
    for (size_t i = 0; i < includes->length; i++) {
      String currInclude = VecAt((*includes), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mateState.arena, "/I\"%s\"", currInclude.data);
        StringBuilderAppend(mateState.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(mateState.arena, " /I\"%s\"", currInclude.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  } else {
    // GCC/Clang format: -I"path"
    for (size_t i = 0; i < includes->length; i++) {
      String currInclude = VecAt((*includes), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mateState.arena, "-I\"%s\"", currInclude.data);
        StringBuilderAppend(mateState.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(mateState.arena, " -I\"%s\"", currInclude.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  }

  *targetIncludes = builder.buffer;
}

static void mateAddFrameworkPaths(String *targetIncludes, StringVector *includes) {
  Assert(isClang() || isGCC(), "AddFrameworkPaths: This function is only supported for GCC/Clang.");

  StringBuilder builder = StringBuilderCreate(mateState.arena);

  if (targetIncludes->length) {
    StringBuilderAppend(mateState.arena, &builder, targetIncludes);
    StringBuilderAppend(mateState.arena, &builder, &S(" "));
  }

  // GCC/Clang format: -F"path"
  for (size_t i = 0; i < includes->length; i++) {
    String currInclude = VecAt((*includes), i);
    if (i == 0 && builder.buffer.length == 0) {
      String buffer = F(mateState.arena, "-F\"%s\"", currInclude.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
      continue;
    }
    String buffer = F(mateState.arena, " -F\"%s\"", currInclude.data);
    StringBuilderAppend(mateState.arena, &builder, &buffer);
  }

  *targetIncludes = builder.buffer;
}

void EndBuild(void) {
  LogInfo("Build took: " FMT_I64 "ms", mateState.totalTime);
  ArenaFree(mateState.arena);

  argParserFreeContext();
}

/* --- Utils Implementation --- */
errno_t RunCommand(String command) {
#if defined(PLATFORM_LINUX) | defined(PLATFORM_MACOS) | defined(PLATFORM_FREEBSD)
  return system(command.data) >> 8;
#else
  return system(command.data);
#endif
}

String CompilerToStr(Compiler compiler) {
  switch (compiler) {
  case GCC:
    return S("gcc");
  case CLANG:
    return S("clang");
  case TCC:
    return S("tcc");
  case MSVC:
    return S("cl.exe");
  default:
    Assert(0, "CompilerToStr: failed, should never get here, compiler given does not exist: %d", compiler);
  }
}

FlagBuilder FlagBuilderCreate(void) {
  return StringBuilderCreate(mateState.arena);
}

static FlagBuilder mateFlagBuilderReserve(size_t count) {
  return StringBuilderReserve(mateState.arena, count);
}

void FlagBuilderAddString(FlagBuilder *builder, String *flag) {
  StringBuilderAppend(mateState.arena, builder, flag);
}

static void mateFlagBuilderAdd(FlagBuilder *builder, StringVector flags) {
  size_t count = 0;
  if (mateState.compiler == MSVC) {
    String *first = VecAtPtr(flags, 0);
    Assert(first->data[0] != '/', "FlagBuilderAdd: failed, flag should not contain /, e.g usage FlagBuilderAdd(\"W4\")");

    if (builder->buffer.length == 0) {
      StringBuilderAppend(mateState.arena, builder, &S("/"));
      StringBuilderAppend(mateState.arena, builder, first);
      count = 1;
    }

    for (size_t i = count; i < flags.length; i++) {
      StringBuilderAppend(mateState.arena, builder, &S(" /"));
      StringBuilderAppend(mateState.arena, builder, VecAtPtr(flags, i));
    }
    return;
  }

  String *first = VecAtPtr(flags, 0);
  Assert(first->data[0] != '-', "FlagBuilderAdd: failed, flag should not contain -, e.g usage FlagBuilderAdd(\"Wall\")");

  if (builder->buffer.length == 0) {
    StringBuilderAppend(mateState.arena, builder, &S("-"));
    StringBuilderAppend(mateState.arena, builder, VecAtPtr(flags, 0));
    count = 1;
  }

  for (size_t i = count; i < flags.length; i++) {
    StringBuilderAppend(mateState.arena, builder, &S(" -"));
    StringBuilderAppend(mateState.arena, builder, VecAtPtr(flags, i));
  }
}

static String mateFixPathExe(String str) {
  String path = NormalizeExePath(mateState.arena, str);
#if defined(PLATFORM_WIN)
  return F(mateState.arena, "%s\\%s", GetCwd(), path.data);
#else
  return F(mateState.arena, "%s/%s", GetCwd(), path.data);
#endif
}

static String mateFixPath(String str) {
  String path = NormalizePath(mateState.arena, str);
#if defined(PLATFORM_WIN)
  return F(mateState.arena, "%s\\%s", GetCwd(), path.data);
#else
  return F(mateState.arena, "%s/%s", GetCwd(), path.data);
#endif
}

static String mateConvertNinjaPath(String str) {
#if defined(PLATFORM_WIN)
  String copy = StrNewSize(mateState.arena, str.data, str.length + 1);
  memmove(&copy.data[2], &copy.data[1], str.length - 1);
  copy.data[1] = '$';
  copy.data[2] = ':';
  return copy;
#else
  return str;
#endif
}

// TODO: Create something like NormalizeOutput
static StringVector mateOutputTransformer(StringVector vector) {
  StringVector result = {0};

  if (isMSVC()) {
    for (size_t i = 0; i < vector.length; i++) {
      String currentExecutable = VecAt(vector, i);
      if (StrIsNull(currentExecutable)) {
        VecPush(result, S(""));
        continue;
      }

      size_t lastCharIndex = 0;
      for (size_t j = currentExecutable.length - 1; j > 0; j--) {
        char currentChar = currentExecutable.data[j];
        if (currentChar == '/') {
          lastCharIndex = j;
          break;
        }
      }

      Assert(lastCharIndex != 0, "MateOutputTransformer: failed to transform %s, to an object file", currentExecutable.data);
      char *filenameStart = currentExecutable.data + lastCharIndex + 1;
      size_t filenameLength = currentExecutable.length - (lastCharIndex + 1);

      String objOutput = StrNewSize(mateState.arena, filenameStart, filenameLength + 2);
      objOutput.data[objOutput.length - 3] = 'o';
      objOutput.data[objOutput.length - 2] = 'b';
      objOutput.data[objOutput.length - 1] = 'j';

      VecPush(result, objOutput);
    }
    return result;
  }

  for (size_t i = 0; i < vector.length; i++) {
    String currentExecutable = VecAt(vector, i);
    if (StrIsNull(currentExecutable)) {
      VecPush(result, S(""));
      continue;
    }

    size_t lastCharIndex = 0;
    for (size_t j = currentExecutable.length - 1; j > 0; j--) {
      char currentChar = currentExecutable.data[j];
      if (currentChar == '/') {
        lastCharIndex = j;
        break;
      }
    }
    Assert(lastCharIndex != 0, "MateOutputTransformer: failed to transform %s, to an object file", currentExecutable.data);
    String output = StrSlice(mateState.arena, currentExecutable, lastCharIndex + 1, currentExecutable.length);
    output.data[output.length - 1] = 'o';
    VecPush(result, output);
  }
  return result;
}

static bool mateGlobMatch(String pattern, String text) {
  if (pattern.length == 1 && pattern.data[0] == '*') {
    return true;
  }

  size_t p = 0;
  size_t t = 0;
  size_t starP = -1;
  size_t starT = -1;
  while (t < text.length) {
    if (p < pattern.length && pattern.data[p] == text.data[t]) {
      p++;
      t++;
    } else if (p < pattern.length && pattern.data[p] == '*') {
      starP = p;
      starT = t;
      p++;
    } else if (starP != (size_t)-1) {
      p = starP + 1;
      t = ++starT;
    } else {
      return false;
    }
  }

  while (p < pattern.length && pattern.data[p] == '*') {
    p++;
  }

  return p == pattern.length;
}

bool isMSVC(void) {
  return mateState.compiler == MSVC;
}

bool isGCC(void) {
  return mateState.compiler == GCC;
}

bool isClang(void) {
  return mateState.compiler == CLANG;
}

bool isTCC(void) {
  return mateState.compiler == TCC;
}
