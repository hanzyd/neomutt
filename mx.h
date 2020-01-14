/**
 * @file
 * API for mailboxes
 *
 * @authors
 * Copyright (C) 1996-2002,2013 Michael R. Elkins <me@mutt.org>
 * Copyright (C) 1999-2002 Thomas Roessler <roessler@does-not-exist.org>
 * Copyright (C) 2017-2018 Richard Russon <rich@flatcap.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MUTT_MX_H
#define MUTT_MX_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "config/lib.h"
#include "core/lib.h"

struct Email;
struct Context;
struct Path;
struct stat;

extern const struct MxOps *mx_ops[];

/* These Config Variables are only used in mx.c */
extern bool          C_KeepFlagged;
extern unsigned char C_MboxType;
extern unsigned char C_Move;
extern char *        C_Trash;

extern struct EnumDef MboxTypeDef;

/* flags for mutt_open_mailbox() */
typedef uint8_t OpenMailboxFlags;   ///< Flags for mutt_open_mailbox(), e.g. #MUTT_NOSORT
#define MUTT_OPEN_NO_FLAGS       0  ///< No flags are set
#define MUTT_NOSORT        (1 << 0) ///< Do not sort the mailbox after opening it
#define MUTT_APPEND        (1 << 1) ///< Open mailbox for appending messages
#define MUTT_READONLY      (1 << 2) ///< Open in read-only mode
#define MUTT_QUIET         (1 << 3) ///< Do not print any messages
#define MUTT_NEWFOLDER     (1 << 4) ///< Create a new folder - same as #MUTT_APPEND,
                                    ///< but uses mutt_file_fopen() with mode "w" for mbox-style folders.
                                    ///< This will truncate an existing file.
#define MUTT_PEEK          (1 << 5) ///< Revert atime back after taking a look (if applicable)
#define MUTT_APPENDNEW     (1 << 6) ///< Set in mx_open_mailbox_append if the mailbox doesn't exist.
                                    ///< Used by maildir/mh to create the mailbox.

typedef uint8_t MsgOpenFlags;      ///< Flags for mx_msg_open_new(), e.g. #MUTT_ADD_FROM
#define MUTT_MSG_NO_FLAGS       0  ///< No flags are set
#define MUTT_ADD_FROM     (1 << 0) ///< add a From_ line
#define MUTT_SET_DRAFT    (1 << 1) ///< set the message draft flag

/**
 * enum MxCheckReturns - Return values from mx_mbox_check(), mx_mbox_sync(), and mx_mbox_close()
 */
enum MxStatus
{
  MX_STATUS_ERROR = -1, ///< An error occurred
  MX_STATUS_OK,         ///< No changes
  MX_STATUS_NEW_MAIL,   ///< New mail received in Mailbox
  MX_STATUS_LOCKED,     ///< Couldn't lock the Mailbox
  MX_STATUS_REOPENED,   ///< Mailbox was reopened
  MX_STATUS_FLAGS,      ///< Nondestructive flags change (IMAP)
};

/**
 * enum MxOpenReturns - Return values for mbox_open()
 */
enum MxOpenReturns
{
  MX_OPEN_OK,           ///< Open succeeded
  MX_OPEN_ERROR,        ///< Open failed with an error
  MX_OPEN_ABORT,        ///< Open was aborted
};

/**
 * struct Message - A local copy of an email
 */
struct Message
{
  FILE *fp;             ///< pointer to the message data
  char *path;           ///< path to temp file
  char *committed_path; ///< the final path generated by mx_msg_commit()
  bool write;           ///< nonzero if message is open for writing
  struct
  {
    bool read : 1;
    bool flagged : 1;
    bool replied : 1;
    bool draft : 1;
  } flags;
  time_t received; ///< the time at which this message was received
};

/**
 * struct MxOps - The Mailbox API
 *
 * Each backend provides a set of functions through which the Mailbox, messages,
 * tags and paths are manipulated.
 */
struct MxOps
{
  enum MailboxType type;  ///< Mailbox type, e.g. #MUTT_IMAP
  const char *name;       ///< Mailbox name, e.g. "imap"
  bool is_local;          ///< True, if Mailbox type has local files/dirs

  /**
   * ac_owns_path - Check whether an Account owns a Mailbox path
   * @param a    Account
   * @param path Mailbox Path
   * @retval true  Account handles path
   * @retval false Account does not handle path
   *
   * **Contract**
   * - @a a    is not NULL
   * - @a path is not NULL
   */
  bool (*ac_owns_path)(struct Account *a, const char *path);

  /**
   * ac_add - Add a Mailbox to an Account
   * @param a Account to add to
   * @param m Mailbox to add
   * @retval  true  Success
   * @retval  false Error
   *
   * **Contract**
   * - @a a is not NULL
   * - @a m is not NULL
   */
  bool (*ac_add)(struct Account *a, struct Mailbox *m);

  /**
   * mbox_open - Open a Mailbox
   * @param m Mailbox to open
   * @retval enum #MxOpenReturns
   *
   * **Contract**
   * - @a m is not NULL
   */
  enum MxOpenReturns (*mbox_open)(struct Mailbox *m);

  /**
   * mbox_open_append - Open a Mailbox for appending
   * @param m     Mailbox to open
   * @param flags Flags, see #OpenMailboxFlags
   * @retval true Success
   * @retval false Failure
   *
   * **Contract**
   * - @a m is not NULL
   */
  bool (*mbox_open_append)(struct Mailbox *m, OpenMailboxFlags flags);

  /**
   * mbox_check - Check for new mail
   * @param m Mailbox
   * @retval enum #MxStatus
   *
   * **Contract**
   * - @a m is not NULL
   */
  enum MxStatus (*mbox_check)(struct Mailbox *m);

  /**
   * mbox_check_stats - Check the Mailbox statistics
   * @param m     Mailbox to check
   * @param flags Function flags
   * @retval enum #MxStatus
   *
   * **Contract**
   * - @a m is not NULL
   */
  enum MxStatus (*mbox_check_stats)(struct Mailbox *m, uint8_t flags);

  /**
   * mbox_sync - Save changes to the Mailbox
   * @param m Mailbox to sync
   * @retval enum #MxStatus
   *
   * **Contract**
   * - @a m is not NULL
   */
  enum MxStatus (*mbox_sync)(struct Mailbox *m);

  /**
   * mbox_close - Close a Mailbox
   * @param m Mailbox to close
   * @retval enum #MxStatus
   *
   * **Contract**
   * - @a m is not NULL
   */
  enum MxStatus (*mbox_close)(struct Mailbox *m);

  /**
   * msg_open - Open an email message in a Mailbox
   * @param m     Mailbox
   * @param msg   Message to open
   * @param msgno Index of message to open
   * @retval true Success
   * @retval false Error
   *
   * **Contract**
   * - @a m   is not NULL
   * - @a msg is not NULL
   * - 0 <= @a msgno < msg->msg_count
   */
  bool (*msg_open)(struct Mailbox *m, struct Message *msg, int msgno);

  /**
   * msg_open_new - Open a new message in a Mailbox
   * @param m   Mailbox
   * @param msg Message to open
   * @param e   Email
   * @retval true Success
   * @retval false Failure
   *
   * **Contract**
   * - @a m   is not NULL
   * - @a msg is not NULL
   */
  bool (*msg_open_new)(struct Mailbox *m, struct Message *msg, const struct Email *e);

  /**
   * msg_commit - Save changes to an email
   * @param m   Mailbox
   * @param msg Message to commit
   * @retval  0 Success
   * @retval -1 Failure
   *
   * **Contract**
   * - @a m   is not NULL
   * - @a msg is not NULL
   */
  int (*msg_commit)      (struct Mailbox *m, struct Message *msg);

  /**
   * msg_close - Close an email
   * @param m   Mailbox
   * @param msg Message to close
   * @retval  0 Success
   * @retval -1 Failure
   *
   * **Contract**
   * - @a m   is not NULL
   * - @a msg is not NULL
   */
  int (*msg_close)       (struct Mailbox *m, struct Message *msg);

  /**
   * msg_padding_size - Bytes of padding between messages
   * @param m Mailbox
   * @retval num Bytes of padding
   *
   * **Contract**
   * - @a m is not NULL
   */
  int (*msg_padding_size)(struct Mailbox *m);

  /**
   * msg_save_hcache - Save message to the header cache
   * @param m Mailbox
   * @param e Email
   * @retval  0 Success
   * @retval -1 Failure
   *
   * **Contract**
   * - @a m is not NULL
   * - @a e is not NULL
   */
  int (*msg_save_hcache) (struct Mailbox *m, struct Email *e);

  /**
   * tags_edit - Prompt and validate new messages tags
   * @param m      Mailbox
   * @param tags   Existing tags
   * @param buf    Buffer to store the tags
   * @param buflen Length of buffer
   * @retval -1 Error
   * @retval  0 No valid user input
   * @retval  1 Buf set
   *
   * **Contract**
   * - @a m   is not NULL
   * - @a buf is not NULL
   */
  int (*tags_edit)       (struct Mailbox *m, const char *tags, char *buf, size_t buflen);

  /**
   * tags_commit - Save the tags to a message
   * @param m Mailbox
   * @param e Email
   * @param buf Buffer containing tags
   * @retval  0 Success
   * @retval -1 Failure
   *
   * **Contract**
   * - @a m   is not NULL
   * - @a e   is not NULL
   * - @a buf is not NULL
   */
  int (*tags_commit)     (struct Mailbox *m, struct Email *e, char *buf);

  /**
   * path_probe - Does this Mailbox type recognise this path?
   * @param path Path to examine
   * @param st   stat buffer (for local filesystems)
   * @retval num Type, e.g. #MUTT_IMAP
   *
   * **Contract**
   * - @a path is not NULL
   */
  enum MailboxType (*path_probe)(const char *path, const struct stat *st);

  /**
   * path_canon - Canonicalise a Mailbox path
   * @param buf    Path to modify
   * @param buflen Length of buffer
   * @retval  0 Success
   * @retval -1 Failure
   *
   * **Contract**
   * - @a buf is not NULL
   */
  int (*path_canon)      (char *buf, size_t buflen);

  /**
   * path_pretty - Abbreviate a Mailbox path
   * @param buf    Path to modify
   * @param buflen Length of buffer
   * @param folder Base path for '=' substitution
   * @retval  0 Success
   * @retval -1 Failure
   *
   * **Contract**
   * - @a buf is not NULL
   */
  int (*path_pretty)     (char *buf, size_t buflen, const char *folder);

  /**
   * path_parent - Find the parent of a Mailbox path
   * @param buf    Path to modify
   * @param buflen Length of buffer
   * @retval  0 Success
   * @retval -1 Failure
   *
   * **Contract**
   * - @a buf is not NULL
   */
  int (*path_parent)     (char *buf, size_t buflen);

  /**
   * path_is_empty - Is the Mailbox empty?
   * @param path Mailbox to check
   * @retval 1 Mailbox is empty
   * @retval 0 Mailbox contains mail
   * @retval -1 Error
   *
   * **Contract**
   * - @a path is not NULL and not empty
   */
  int (*path_is_empty)     (const char *path);

  /**
   * path2_canon - Canonicalise a Mailbox path
   * @param path Path to canonicalise
   * @retval  0 Success
   * @retval -1 Failure
   *
   * **Contract**
   * - @a path        is not NULL
   * - @a path->orig  is not NULL
   * - @a path->canon is NULL
   * - @a path->type  is not #MUTT_UNKNOWN
   * - @a path->flags has #MPATH_RESOLVED, #MPATH_TIDY
   * - @a path->flags does not have #MPATH_CANONICAL
   *
   * **On success**
   * - @a path->canon will be set to a new string
   * - @a path->flags will have #MPATH_CANONICAL added
   */
  int (*path2_canon)(struct Path *path);

  /**
   * path2_compare - Compare two Mailbox paths
   * @param path1 First  path to compare
   * @param path2 Second path to compare
   * @retval -1 path1 precedes path2
   * @retval  0 path1 and path2 are identical
   * @retval  1 path2 precedes path1
   *
   * **Contract**
   * - @a path        is not NULL
   * - @a path->canon is not NULL
   * - @a path->type  is not #MUTT_UNKNOWN
   * - @a path->flags has #MPATH_RESOLVED, #MPATH_TIDY, #MPATH_CANONICAL
   *
   * The contract applies to both @a path1 and @a path2
   */
  int (*path2_compare)(struct Path *path1, struct Path *path2);

  /**
   * path2_parent - Find the parent of a Mailbox path
   * @param[in] path    Mailbox path
   * @param[out] parent Parent of path
   * @retval  0 Success, parent returned
   * @retval -1 Failure, path is root, it has no parent
   * @retval -2 Error
   *
   * **Contract**
   * - @a path        is not NULL
   * - @a path->orig  is not NULL
   * - @a path->type  is not #MUTT_UNKNOWN
   * - @a path->flags has #MPATH_RESOLVED, #MPATH_TIDY
   * - @a parent      is not NULL
   * - @a *parent     is NULL
   *
   * **On success**
   * - @a parent        is set to a new Path
   * - @a parent->orig  is set to the parent
   * - @a parent->type  is set to @a path->type
   * - @a parent->flags has #MPATH_RESOLVED, #MPATH_TIDY
   *
   * @note The caller must free the returned Path
   */
  int (*path2_parent)(const struct Path *path, struct Path **parent);

  /**
   * path2_pretty - Abbreviate a Mailbox path
   * @param[in]  path   Mailbox path
   * @param[in]  folder Folder string to abbreviate with
   * @retval  1 Success, Path abbreviated
   * @retval  0 Failure, No change possible
   * @retval -1 Error
   *
   * **Contract**
   * - @a path         is not NULL
   * - @a path->orig   is not NULL
   * - @a path->pretty is NULL
   * - @a path->type   is not #MUTT_UNKNOWN
   * - @a path->flags  has #MPATH_RESOLVED, #MPATH_TIDY
   * - @a folder       is not NULL, is not empty
   *
   * **On success**
   * - @a path->pretty is set to the abbreviated path
   *
   * @note The caller must free the returned Path
   */
  int (*path2_pretty)(struct Path *path, const char *folder);

  /**
   * path2_probe - Does this Mailbox type recognise this path?
   * @param path Path to examine
   * @param st   stat buffer (for local filesystems)
   * @retval  0 Success, path recognised
   * @retval -1 Error or path not recognised
   *
   * **Contract**
   * - @a path        is not NULL
   * - @a path->orig  is not NULL
   * - @a path->canon is NULL
   * - @a path->type  is #MUTT_UNKNOWN
   * - @a path->flags has #MPATH_RESOLVED, #MPATH_TIDY
   * - @a path->flags does not have #MPATH_CANONICAL
   * - @a st          is not NULL if MxOps::is_local is set
   *
   * **On success**
   * - @a path->type is set, e.g. #MUTT_MAILDIR
   */
  int (*path2_probe)(struct Path *path, const struct stat *st);

  /**
   * path2_tidy - Tidy a Mailbox path
   * @param path Path to tidy
   * @retval  0 Success
   * @retval -1 Failure
   *
   * **Contract**
   * - @a path        is not NULL
   * - @a path->orig  is not NULL
   * - @a path->canon is NULL
   * - @a path->type  is not #MUTT_UNKNOWN
   * - @a path->flags has #MPATH_RESOLVED
   * - @a path->flags does not have #MPATH_TIDY, #MPATH_CANONICAL
   *
   * **On success**
   * - @a path->orig  will be replaced by a tidier string
   * - @a path->flags will have #MPATH_TIDY added
   */
  int (*path2_tidy)(struct Path *path);

  /**
   * ac2_is_owner - Does this Account own this Path?
   * @param a    Account to search
   * @param path Path to search for
   * @retval  0 Success
   * @retval -1 Error
   *
   * **Contract**
   * - @a a           is not NULL
   * - @a a->magic    matches the backend
   * - @a path        is not NULL
   * - @a path->orig  is not NULL
   * - @a path->type  matches the backend
   * - @a path->flags has #MPATH_RESOLVED, #MPATH_TIDY
   */
  int (*ac2_is_owner)(struct Account *a, const struct Path *path);
};

/* Wrappers for the Mailbox API, see MxOps */
enum MxStatus   mx_mbox_check      (struct Mailbox *m);
enum MxStatus   mx_mbox_check_stats(struct Mailbox *m, uint8_t flags);
enum MxStatus   mx_mbox_close      (struct Context **ptr);
struct Context *mx_mbox_open       (struct Mailbox *m, OpenMailboxFlags flags);
enum MxStatus   mx_mbox_sync       (struct Mailbox *m);
int             mx_msg_close       (struct Mailbox *m, struct Message **msg);
int             mx_msg_commit      (struct Mailbox *m, struct Message *msg);
struct Message *mx_msg_open_new    (struct Mailbox *m, const struct Email *e, MsgOpenFlags flags);
struct Message *mx_msg_open        (struct Mailbox *m, int msgno);
int             mx_msg_padding_size(struct Mailbox *m);
int             mx_save_hcache     (struct Mailbox *m, struct Email *e);
int             mx_path_canon      (char *buf, size_t buflen, const char *folder, enum MailboxType *type);
int             mx_path_canon2     (struct Mailbox *m, const char *folder);
int             mx_path_parent     (char *buf, size_t buflen);
int             mx_path_pretty     (char *buf, size_t buflen, const char *folder);
enum MailboxType mx_path_probe     (const char *path);
struct Mailbox *mx_path_resolve    (const char *path, const char *folder);
struct Mailbox *mx_resolve         (const char *path_or_name);
int             mx_tags_commit     (struct Mailbox *m, struct Email *e, char *tags);
int             mx_tags_edit       (struct Mailbox *m, const char *tags, char *buf, size_t buflen);

struct Account *mx_ac_find     (struct Mailbox *m);
struct Mailbox *mx_mbox_find   (struct Account *a, const char *path);
struct Mailbox *mx_mbox_find2  (const char *path, const char *folder);
bool            mx_mbox_ac_link(struct Mailbox *m);
bool            mx_ac_add      (struct Account *a, struct Mailbox *m);
int             mx_ac_remove   (struct Mailbox *m);

int                 mx_access           (const char *path, int flags);
void                mx_alloc_memory     (struct Mailbox *m);
int                 mx_path_is_empty    (const char *path);
void                mx_fastclose_mailbox(struct Mailbox *m);
const struct MxOps *mx_get_ops          (enum MailboxType type);
bool                mx_tags_is_supported(struct Mailbox *m);

int              mx_path2_canon  (struct Path *path);
int              mx_path2_compare(struct Path *path1, struct Path *path2);
int              mx_path2_parent (const struct Path *path, struct Path **parent);
int              mx_path2_pretty (struct Path *path, const char *folder);
enum MailboxType mx_path2_probe  (struct Path *path);
int              mx_path2_resolve(struct Path *path);

#endif /* MUTT_MX_H */
