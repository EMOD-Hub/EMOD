#define STRINGIFY(ver)       #ver
#define XSTRINGIFY(ver)       STRINGIFY(ver)

#ifdef WIN32
static const char* BUILD_DATE = __DATE__ " " __TIME__;
#else
#define BUILD_DATE __DATE__
#endif

#ifndef SCCS_BRANCH
#define    SCCS_BRANCH      "unknown-branch (unknown)"
#endif
#ifndef SCCS_DATE
#define    SCCS_DATE        "date time unknown"
#endif

#define    MAJOR_VERSION    2
#define    MINOR_VERSION    20
#ifndef REVISION_NUMBER
#define    REVISION_NUMBER  00
#endif
#define    BUILD_NUMBER     0

#ifndef BUILDER_NAME
#define    BUILDER_NAME     "danbr"
#endif

#define FULL_VERSION_WITH_SVN       XSTRINGIFY(MAJOR_VERSION) "." XSTRINGIFY(MINOR_VERSION) "." XSTRINGIFY(REVISION_NUMBER)
#define FULL_VERSION_WITH_SVN_NQ    MAJOR_VERSION,MINOR_VERSION,REVISION_NUMBER,BUILD_NUMBER


// Semantic versioning for serialized population, Major.Minor.Patch
//1.0.0 - 12/05/2022 - PR-4807 - Initial version
//2.0.0 - 01/12/2023 - GH-4846 - Added sim_time_created to Infection
//3.0.0 - 02/03/2023 - GH-4865 - Added m_BiteID to StrainIdentityMalariaGenetics
//4.0.0 - 06/30/2023 - GH-4935 - FPG-Hash Collision - New genome ID generator in ParasiteGenetics a new ID in a genome
//5.0.0 - 07/09/2023 - GH-4792 - Added m_OocystDuration to ParasiteCohort

#define SER_POP_MAJOR_VERSION   5
#define SER_POP_MINOR_VERSION   0
#define SER_POP_PATCH_VERSION   0

