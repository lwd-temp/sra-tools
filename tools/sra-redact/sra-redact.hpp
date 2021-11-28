/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#include <kapp/main.h>
#include <kapp/args.h>
#include <vdb/manager.h>
#include <vdb/database.h>
#include <vdb/table.h>
#include <vdb/cursor.h>
#include <vdb/schema.h>
#include <vdb/vdb-priv.h>
#include <kdb/manager.h>
#include <kdb/meta.h>
#include <kfs/directory.h>
#include <klib/log.h>
#include <klib/out.h>
#include <klib/rc.h>
#include <klib/data-buffer.h>
#include <klib/printf.h>
#include <sra/sradb.h>

#include <stdarg.h>
#include <assert.h>
#include <inttypes.h>

#include <sysexits.h>
#include <unistd.h>
#include <sys/mman.h> 

#define TEMP_MAIN_OBJECT_NAME "out"

extern "C" {
    rc_t KMDataNodeAddr(const KMDataNode *self, const void **addr, size_t *size);
}

struct CellData {
    void const *data;
    uint32_t count;
    uint32_t elem_bits;

    CellData()
    : data(nullptr)
    , count(0)
    , elem_bits(0)
    {}

    template<typename T>
    class Typed {
        friend CellData;

        T const *data_;
        size_t count_;

        Typed(CellData const &parent) {
            assert(parent.elem_bits == sizeof(T) * 8);
            data_ = reinterpret_cast<T const *>(parent.data);
            count_ = parent.count;
        }
    public:
        T const *data() const { return data_; }
        size_t count() const { return count_; }
        T const *begin() const { return data_; }
        T const *end() const { return data_ + count(); }

        T const &operator [](int i) const {
            assert(i >= 0 && (size_t)i < count());
            return data_[i];
        }
        T const &front() const { return data_[0]; }
        T const &back() const { return data_[count_ - 1]; }
    };

    template<typename T>
    Typed<T> const typed() const {
        return Typed<T>(*this);
    }
};

static CellData cellData(char const *const colName, int const col, int64_t const row, VCursor const *const curs);
static uint64_t rowCount(VCursor const *const curs, int64_t *const first, uint32_t cid);
static uint32_t addColumn( char const *const name
                         , char const *const type
                         , VCursor const *const curs);
static void openCursor(VCursor const *const curs, char const *const name);
static void openRow(int64_t const row, VCursor const *const out);
static void commitRow(int64_t const row, VCursor *const out);
static void closeRow(int64_t const row, VCursor *const out);
static void commitCursor(VCursor *const out);
static VDBManager *manager();
static Args *getArgs(int argc, char *argv[]);
static char const *getOptArgValue(int opt, Args *const args);
static char const *getParameter(Args *const args, char const *);
static int pathType(VDBManager const *mgr, char const *path);
static void tblSchemaInfo(VTable const *tbl, char **name, VSchema *schema);
static void dbSchemaInfo(VDatabase const *db, char **name, VSchema *schema);

struct Inputs {
    VTable const *sequence = nullptr;
    VTable const *primaryAlignment = nullptr;
    VTable const *secondaryAlignment = nullptr;
    std::string schemaType;
    bool noDb = false;
};

struct Output {
    VTable *tbl = nullptr;
    std::vector<std::string> changedColumns;
};

struct Outputs {
    Output sequence;
    Output primaryAlignment;
    Output secondaryAlignment;
};

static Inputs openInputs(char const *input, VDBManager const *mgr, VSchema *schema);
static bool processSequenceTable(Output &output, VTable const *const input, bool const aligned);
static void processAlignmentTable(VTable const *const input);
static void redactAlignmentTable(Output &output, VTable const *const input, bool const isPrimary);
static Outputs createOutputs(Args *const args, VDBManager *const mgr, Inputs const &inputs, VSchema const *schema);
static VSchema *makeSchema(VDBManager *mgr);

static void OUT_OF_MEMORY()
{
    LogErr(klogFatal, RC(rcExe, rcFile, rcReading, rcMemory, rcExhausted), "OUT OF MEMORY!!!");
    exit(EX_TEMPFAIL);
}

#define WARN_IF(CALL) do { \
    auto const rc = CALL; \
    if (rc != 0) \
        LogErr(klogWarn, rc, "Failed: " #CALL ); \
} while(0)

#define DIE_UNLESS(EX, CALL) do { \
    auto const rc = CALL; \
    if (rc == 0) break; \
    LogErr(klogFatal, rc, "FAILED! " #CALL); \
    exit(EX); \
} while(0)

static CellData cellData(char const *const colName, int const col, int64_t const row, VCursor const *const curs)
{
    CellData result;

    if (col) {
        uint32_t bit_offset = 0;
        rc_t rc = VCursorCellDataDirect(curs, row, col, &result.elem_bits, &result.data, &bit_offset, &result.count);
        if (rc) {
            pLogErr(klogFatal, rc, "Failed to read $(col) at row $(row)", "col=%s,row=%ld", colName, row);
            exit(EX_DATAERR);
        }
        assert(bit_offset == 0);
        assert((result.elem_bits & 0x7) == 0);
    }
    return result;
}

static uint64_t rowCount(VCursor const *const curs, int64_t *const first, uint32_t cid)
{
    uint64_t count = 0;
    DIE_UNLESS(EX_SOFTWARE, VCursorIdRange(curs, cid, first, &count));
    return count;
}

static uint32_t addColumn( char const *const name
                          , VCursor const *const curs)
{
    uint32_t cid = 0;
    rc_t const rc = VCursorAddColumn(curs, &cid, "%s", name);
    if (rc == 0)
        return cid;

    pLogErr(klogFatal, rc, "Failed to open $(name) column", "name=%s", name);
    exit(EX_NOINPUT);
}

static uint32_t addColumn( char const *const name
                         , char const *const type
                         , VCursor const *const curs)
{
    uint32_t cid = 0;
    rc_t const rc = VCursorAddColumn(curs, &cid, "(%s)%s", type, name);
    if (rc == 0)
        return cid;

    pLogErr(klogFatal, rc, "Failed to open $(name) column", "name=%s", name);
    exit(EX_NOINPUT);
}

static uint32_t addColumn(  char const *const name
                          , VCursor const *const curs
                          , bool &have)
{
    uint32_t cid = 0;
    rc_t const rc = VCursorAddColumn(curs, &cid, "%s", name);
    have = rc == 0;
    return rc == 0 ? cid : 0;
}

static uint32_t addColumn(  char const *const name
                          , char const *const type
                          , VCursor const *const curs
                          , bool &have)
{
    uint32_t cid = 0;
    rc_t const rc = VCursorAddColumn(curs, &cid, "(%s)%s", type, name);
    have = rc == 0;
    return rc == 0 ? cid : 0;
}

static uint32_t addColumn(  char const *const name
                          , char const *const altname
                          , char const *const type
                          , VCursor const *const curs
                          , char const *&used)
{
    auto have = false;
    auto cid = addColumn(used = name, type, curs, have);
    if (have)
        return cid;
    cid = addColumn(used = altname, type, curs, have);
    if (have)
        return cid;
    used = nullptr;
    return 0;
}

static void openCursor(VCursor const *const curs, char const *const name)
{
    rc_t const rc = VCursorOpen(curs);
    if (rc) {
        pLogErr(klogFatal, rc, "Failed to open $(name) cursor", "name=%s", name);
        exit(EX_NOINPUT);
    }
}

static VTable const *openTable(char const *const name, VDBManager const *const mgr)
{
    VTable const *in = NULL;
    rc_t const rc = VDBManagerOpenTableRead(mgr, &in, NULL, "%s", name);
    if (rc == 0)
        return in;

    pLogErr(klogFatal, rc, "can't open input table $(name)", "name=%s", name);
    exit(EX_SOFTWARE);
}

static VTable const *openInputTable(char const *const name, VDBManager const *const mgr, std::string &schemaType, VSchema *schema)
{
    VTable const *const in = openTable(name, mgr);
    char *schemaName;

    tblSchemaInfo(in, &schemaName, schema);
    schemaType.assign(schemaName);
    free(schemaName);

    return in;
}

static VDatabase const *openDatabase(char const *const input, VDBManager const *const mgr)
{
    VDatabase const *db = NULL;
    rc_t const rc = VDBManagerOpenDBRead(mgr, &db, NULL, "%s", input);
    if (rc == 0)
        return db;
        
    LogErr(klogFatal, rc, "can't open input database");
    exit(EX_SOFTWARE);
}

static void getSchemaInfo(KMetadata const *const meta, char **type, VSchema *schema)
{
    KMDataNode const *root = NULL;
    KMDataNode const *node = NULL;
    size_t valueLen = 0;
    char *value = NULL;

    DIE_UNLESS(EX_SOFTWARE, KMetadataOpenNodeRead(meta, &root, NULL)); KMetadataRelease(meta);
    DIE_UNLESS(EX_SOFTWARE, KMDataNodeOpenNodeRead(root, &node, "schema")); KMDataNodeRelease(root);
    DIE_UNLESS(EX_SOFTWARE, KMDataNodeAddr(node, (const void **)&value, &valueLen));
    DIE_UNLESS(EX_SOFTWARE, VSchemaParseText(schema, NULL, value, valueLen));
    {
        char dummy = 0;
        rc_t const rc = KMDataNodeReadAttr(node, "name", &dummy, 0, &valueLen);
        assert(GetRCObject(rc) == (int)rcBuffer && GetRCState(rc) == (int)rcInsufficient);
        if (!(GetRCObject(rc) == (int)rcBuffer && GetRCState(rc) == (int)rcInsufficient)) {
            LogErr(klogFatal, rc, "can't get database schema");
            exit(EX_SOFTWARE);
        }
    }
    value = (char *)malloc(valueLen + 1);
    if (value == NULL)
        OUT_OF_MEMORY();
    DIE_UNLESS(EX_SOFTWARE, KMDataNodeReadAttr(node, "name", value, valueLen + 1, &valueLen));
    KMDataNodeRelease(node);

    value[valueLen] = '\0';
    *type = value;
    pLogMsg(klogInfo, "Schema type is $(type)", "type=%s", value);
}

static void dbSchemaInfo(VDatabase const *const db, char **name, VSchema *schema)
{
    KMetadata const *meta = NULL;
    DIE_UNLESS(EX_SOFTWARE, VDatabaseOpenMetadataRead(db, &meta));
    getSchemaInfo(meta, name, schema);
}

static void tblSchemaInfo(VTable const *const tbl, char **name, VSchema *schema)
{
    KMetadata const *meta = NULL;
    DIE_UNLESS(EX_SOFTWARE, VTableOpenMetadataRead(tbl, &meta));
    getSchemaInfo(meta, name, schema);
}

static VTable const *dbOpenTable(  VDatabase const *db
                                 , char const *const table
                                 , VDBManager const *const mgr
                                 , std::string &schemaType
                                 , VSchema *schema
                                 , bool optional = false
                                 )
{
    VTable const *in = NULL;
    rc_t const rc = VDatabaseOpenTableRead(db, &in, "%s", table);
    if (rc == 0 && schema) {
        char *schemaName;
        dbSchemaInfo(db, &schemaName, schema);
        schemaType.assign(schemaName);
        free(schemaName);
    }
    if (rc == 0 || optional)
        return in;

    LogErr(klogFatal, rc, "can't open input table");
    exit(EX_NOINPUT);
}

#define PATH_TYPE_ISA_DATABASE(TYPE) ((TYPE | kptAlias) == (kptDatabase | kptAlias))
#define PATH_TYPE_ISA_TABLE(TYPE) ((TYPE | kptAlias) == (kptTable | kptAlias))

static VDBManager *manager()
{
    VDBManager *mgr = NULL;
    DIE_UNLESS(EX_SOFTWARE, VDBManagerMakeUpdate(&mgr, NULL));
    return mgr;
}

static int pathType(VDBManager const *mgr, char const *path)
{
    KDBManager const *kmgr = 0;
    DIE_UNLESS(EX_SOFTWARE, VDBManagerOpenKDBManagerRead(mgr, &kmgr));
    auto const type = KDBManagerPathType(kmgr, "%s", path);
    KDBManagerRelease(kmgr);
    return type;
}

static void openRow(int64_t const row, VCursor const *const out)
{
    DIE_UNLESS(EX_IOERR, VCursorOpenRow(out));
}

static void commitRow(int64_t const row, VCursor *const out)
{
    DIE_UNLESS(EX_IOERR, VCursorCommitRow(out));
}

static void closeRow(int64_t const row, VCursor *const out)
{
    DIE_UNLESS(EX_IOERR, VCursorCloseRow(out));
}

static void commitCursor(VCursor *const out)
{
    WARN_IF(VCursorFlushPage(out));
    DIE_UNLESS(EX_IOERR, VCursorCommit(out));
}

static void writeRow(  int64_t const row
                      , CellData const &data
                      , uint32_t const cid
                      , VCursor *const out)
{
    if (cid) {
        DIE_UNLESS(EX_IOERR, VCursorWrite(out, cid, data.elem_bits, data.data, 0, data.count));
    }
}

template <typename T>
static void writeRow(  int64_t const row
                     , std::vector<T> const &data
                     , uint32_t const cid
                     , VCursor *const curs)
{
    if (cid) {
        DIE_UNLESS(EX_IOERR, VCursorWrite(curs, cid, 8 * sizeof(data.front()), data.data(), 0, data.size()));
    }
}

static KDirectory *rootDir()
{
    KDirectory *ndir = NULL;
    DIE_UNLESS(EX_SOFTWARE, KDirectoryNativeDir(&ndir));
    return ndir;
}

static KDirectory *openDirUpdate(char const *const path, ...)
{
    va_list va;
    KDirectory *ndir = rootDir();
    KDirectory *result = NULL;
    rc_t rc;

    va_start(va, path);
    rc = KDirectoryVOpenDirUpdate(ndir, &result, false, path, va);
    va_end(va);
    KDirectoryRelease(ndir);
    if (rc == 0)
        return result;

    LogErr(klogFatal, rc, "Can't get directory");
    exit(EX_SOFTWARE);
}

static KDirectory const *openDirRead(char const *const path, ...)
{
    va_list va;
    KDirectory *ndir = rootDir();
    KDirectory const *result = NULL;
    rc_t rc;

    va_start(va, path);
    rc = KDirectoryVOpenDirRead(ndir, &result, false, path, va);
    va_end(va);
    KDirectoryRelease(ndir);
    if (rc == 0)
        return result;

    LogErr(klogFatal, rc, "Can't get directory");
    exit(EX_SOFTWARE);
}

template <typename F>
void KNamelistForEach(KNamelist const *const list, F && f) {
    uint32_t count = 0;
    DIE_UNLESS(EX_SOFTWARE, KNamelistCount(list, &count));
    for (uint32_t i = 0; i < count; ++i) {
        char const *name = nullptr;
        DIE_UNLESS(EX_SOFTWARE, KNamelistGet(list, i, &name));
        f(name);
    }
}

static void listDir(KDirectory const *const dir) {
    KNamelist *list = nullptr;
    DIE_UNLESS(EX_SOFTWARE, KDirectoryList_v1(dir, &list, nullptr, nullptr, nullptr));
    KNamelistForEach(list, [](char const *name) {
        LogMsg(klogDebug, name);
    });
    KNamelistRelease(list);
}

static KMDataNode const *openNodeRead(VTable const *const tbl, char const *const path, ...)
{
    va_list va;
    KMetadata const *meta = NULL;
    KMDataNode const *node = NULL;
    DIE_UNLESS(EX_SOFTWARE, VTableOpenMetadataRead(tbl, &meta));

    va_start(va, path);
    DIE_UNLESS(EX_SOFTWARE, KMetadataVOpenNodeRead(meta, &node, path, va));
    va_end(va);
    KMetadataRelease(meta);

    return node;
}

static KMDataNode *openNodeUpdate(KMetadata *meta, char const *const path, va_list va) {
    KMDataNode *node = NULL;
    DIE_UNLESS(EX_DATAERR, KMetadataVOpenNodeUpdate(meta, &node, path, va));
    return node;
}

static KMDataNode *openNodeUpdate(VDatabase *const db, char const *const path, ...)
{
    va_list va;
    KMetadata *meta = NULL;
    DIE_UNLESS(EX_SOFTWARE, VDatabaseOpenMetadataUpdate(db, &meta));

    va_start(va, path);
    KMDataNode *const node = openNodeUpdate(meta, path, va);
    va_end(va);
    KMetadataRelease(meta);

    return node;
}

static KMDataNode *openNodeUpdate(VTable *const tbl, char const *const path, ...)
{
    va_list va;
    KMetadata *meta = NULL;
    DIE_UNLESS(EX_SOFTWARE, VTableOpenMetadataUpdate(tbl, &meta));

    va_start(va, path);
    KMDataNode *const node = openNodeUpdate(meta, path, va);
    va_end(va);
    KMetadataRelease(meta);

    return node;
}

static void copyNodeValue(KMDataNode *const dst, KMDataNode const *const src)
{
    extern rc_t KMDataNodeAddr(const KMDataNode *self, const void **addr, size_t *size);
    void const *data = NULL;
    size_t data_size = 0;

    DIE_UNLESS(EX_SOFTWARE, KMDataNodeAddr(src, &data, &data_size));
    DIE_UNLESS(EX_DATAERR, KMDataNodeWrite(dst, data, data_size));

    KMDataNodeRelease(src);
    KMDataNodeRelease(dst);
}

static void writeChildNode(KMDataNode *const node, char const *const name, size_t const size, void const *const data)
{
    KMDataNode *child = NULL;
    DIE_UNLESS(EX_DATAERR, KMDataNodeOpenNodeUpdate(node, &child, "%s", name));
    DIE_UNLESS(EX_DATAERR, KMDataNodeWrite(child, data, size));
    KMDataNodeRelease(child);
}

static VTable *openUpdateTbl(char const *const name, VDBManager *const mgr)
{
    VTable *tbl = NULL;
    DIE_UNLESS(EX_DATAERR, VDBManagerOpenTableUpdate(mgr, &tbl, NULL, "%s", name));
    return tbl;
}

static VDatabase *openUpdateDb(char const *const name, VDBManager *const mgr)
{
    VDatabase *db = NULL;
    DIE_UNLESS(EX_DATAERR, VDBManagerOpenDBUpdate(mgr, &db, NULL, "%s", name));
    return db;
}

static VTable *openUpdateDb(char const *const name, char const *const table, VDBManager *const mgr)
{
    VTable *tbl = NULL;
    VDatabase *db = openUpdateDb(name, mgr);
    DIE_UNLESS(EX_DATAERR, VDatabaseOpenTableUpdate(db, &tbl, "%s", table));
    VDatabaseRelease(db);
    return tbl;
}

static VTable const *openReadDb(char const *const name, char const *const table, VDBManager const *const mgr)
{
    VTable const *tbl = NULL;
    VDatabase const *db = NULL;
    DIE_UNLESS(EX_DATAERR, VDBManagerOpenDBRead(mgr, &db, NULL, "%s", name));
    DIE_UNLESS(EX_DATAERR, VDatabaseOpenTableRead(db, &tbl, "%s", table));
    VDatabaseRelease(db);
    return tbl;
}

static VTable const *openReadTbl(char const *const name, VDBManager const *const mgr)
{
    return openTable(name, mgr);
}

static bool dropColumn(VTable *const tbl, char const *const name)
{
    auto const rc = VTableDropColumn(tbl, "%s", name);
    auto const notFound = GetRCObject(rc) == (int)rcPath && GetRCState(rc) == (int)rcNotFound;
    if (notFound) {
        pLogMsg(klogDebug, "column $(column) doesn't exist", "column=%s", name);
        return false;
    }
    if (rc) {
        pLogErr(klogInfo, rc, "can't drop $(column) column", "column=%s", name);
        exit(EX_SOFTWARE);
    }
    return true;
}

static void removeTempDir(char const *const temp)
{
    KDirectory *ndir = rootDir();
    rc_t rc;
    
    rc = KDirectoryRemove(ndir, true, "%s", temp);
    KDirectoryRelease(ndir);
    if (GetRCState(rc) == (int)rcBusy && GetRCObject(rc) == (int)rcPath) {
        pLogErr(klogWarn, rc, "failed to delete temp object directory $(path), remove it manually", "path=%s", temp);
    }
    else if (rc) {
        LogErr(klogFatal, rc, "failed to delete temp object directory");
        exit(EX_DATAERR);
    }
    else {
        pLogMsg(klogInfo, "Deleted temp object directory $(temp)", "temp=%s", temp);
    }
}
