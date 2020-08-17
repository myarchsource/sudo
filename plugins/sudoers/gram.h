#define COMMAND 257
#define ALIAS 258
#define DEFVAR 259
#define NTWKADDR 260
#define NETGROUP 261
#define USERGROUP 262
#define WORD 263
#define DIGEST 264
#define INCLUDE 265
#define INCLUDEDIR 266
#define DEFAULTS 267
#define DEFAULTS_HOST 268
#define DEFAULTS_USER 269
#define DEFAULTS_RUNAS 270
#define DEFAULTS_CMND 271
#define NOPASSWD 272
#define PASSWD 273
#define NOEXEC 274
#define EXEC 275
#define SETENV 276
#define NOSETENV 277
#define LOG_INPUT 278
#define NOLOG_INPUT 279
#define LOG_OUTPUT 280
#define NOLOG_OUTPUT 281
#define MAIL 282
#define NOMAIL 283
#define FOLLOWLNK 284
#define NOFOLLOWLNK 285
#define ALL 286
#define COMMENT 287
#define HOSTALIAS 288
#define CMNDALIAS 289
#define USERALIAS 290
#define RUNASALIAS 291
#define ERROR 292
#define TYPE 293
#define ROLE 294
#define PRIVS 295
#define LIMITPRIVS 296
#define CMND_TIMEOUT 297
#define NOTBEFORE 298
#define NOTAFTER 299
#define MYSELF 300
#define SHA224_TOK 301
#define SHA256_TOK 302
#define SHA384_TOK 303
#define SHA512_TOK 304
#ifndef YYSTYPE_DEFINED
#define YYSTYPE_DEFINED
typedef union {
    struct cmndspec *cmndspec;
    struct defaults *defaults;
    struct member *member;
    struct runascontainer *runas;
    struct privilege *privilege;
    struct command_digest *digest;
    struct sudo_command command;
    struct command_options options;
    struct cmndtag tag;
    char *string;
    int tok;
} YYSTYPE;
#endif /* YYSTYPE_DEFINED */
extern YYSTYPE sudoerslval;
