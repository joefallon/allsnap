#ifndef ALLSNAP_VERSION_H
#define ALLSNAP_VERSION_H

/* Release version shared by startup, About dialogs, and EXE/DLL resources. */
#define ALLSNAP_VERSION_TEXT "2.0.0"
#define ALLSNAP_VERSION_RESOURCE 2,0,0,0

/* RC dialog labels require one literal; acceptance tests check its version. */
#define ALLSNAP_ABOUT_TEXT "allSnap Version 2.0.0\nIvan Heckman\nCopyright (C) 2002-2010"

/* Numeric token for the existing same-architecture instance notification. */
#define ALLSNAP_INSTANCE_VERSION 0x020000

#endif
