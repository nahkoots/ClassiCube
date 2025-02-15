#include "LWeb.h"
#include "Launcher.h"
#include "Platform.h"
#include "Stream.h"
#include "Logger.h"

#ifndef CC_BUILD_WEB
/*########################################################################################################################*
*----------------------------------------------------------JSON-----------------------------------------------------------*
*#########################################################################################################################*/
#define TOKEN_NONE  0
#define TOKEN_NUM   1
#define TOKEN_TRUE  2
#define TOKEN_FALSE 3
#define TOKEN_NULL  4
/* Consumes n characters from the JSON stream */
#define JsonContext_Consume(ctx, n) ctx->cur += n; ctx->left -= n;

static const String strTrue  = String_FromConst("true");
static const String strFalse = String_FromConst("false");
static const String strNull  = String_FromConst("null");

static bool Json_IsWhitespace(char c) {
	return c == '\r' || c == '\n' || c == '\t' || c == ' ';
}

static bool Json_IsNumber(char c) {
	return c == '-' || c == '.' || (c >= '0' && c <= '9');
}

static bool Json_ConsumeConstant(struct JsonContext* ctx, const String* value) {
	int i;
	if (value->length > ctx->left) return false;

	for (i = 0; i < value->length; i++) {
		if (ctx->cur[i] != value->buffer[i]) return false;
	}

	JsonContext_Consume(ctx, value->length);
	return true;
}

static int Json_ConsumeToken(struct JsonContext* ctx) {
	char c;
	for (; ctx->left && Json_IsWhitespace(*ctx->cur); ) { JsonContext_Consume(ctx, 1); }
	if (!ctx->left) return TOKEN_NONE;

	c = *ctx->cur;
	if (c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == '"' || c == ':') {
		JsonContext_Consume(ctx, 1); return c;
	}

	/* number token forms part of value, don't consume it */
	if (Json_IsNumber(c)) return TOKEN_NUM;

	if (Json_ConsumeConstant(ctx, &strTrue))  return TOKEN_TRUE;
	if (Json_ConsumeConstant(ctx, &strFalse)) return TOKEN_FALSE;
	if (Json_ConsumeConstant(ctx, &strNull))  return TOKEN_NULL;

	/* invalid token */
	JsonContext_Consume(ctx, 1);
	return TOKEN_NONE;
}

static String Json_ConsumeNumber(struct JsonContext* ctx) {
	int len = 0;
	for (; ctx->left && Json_IsNumber(*ctx->cur); len++) { JsonContext_Consume(ctx, 1); }
	return String_Init(ctx->cur - len, len, len);
}

static void Json_ConsumeString(struct JsonContext* ctx, String* str) {
	int codepoint, h[4];
	char c;
	str->length = 0;

	for (; ctx->left;) {
		c = *ctx->cur; JsonContext_Consume(ctx, 1);
		if (c == '"') return;
		if (c != '\\') { String_Append(str, c); continue; }

		/* form of \X */
		if (!ctx->left) break;
		c = *ctx->cur; JsonContext_Consume(ctx, 1);
		if (c == '/' || c == '\\' || c == '"') { String_Append(str, c); continue; }

		/* form of \uYYYY */
		if (c != 'u' || ctx->left < 4) break;
		if (!PackedCol_Unhex(ctx->cur, h, 4)) break;

		codepoint = (h[0] << 12) | (h[1] << 8) | (h[2] << 4) | h[3];
		/* don't want control characters in names/software */
		/* TODO: Convert to CP437.. */
		if (codepoint >= 32) String_Append(str, codepoint);
		JsonContext_Consume(ctx, 4);
	}

	ctx->failed = true; str->length = 0;
}
static String Json_ConsumeValue(int token, struct JsonContext* ctx);

static void Json_ConsumeObject(struct JsonContext* ctx) {
	char keyBuffer[STRING_SIZE];
	String value, oldKey = ctx->curKey;
	int token;
	ctx->OnNewObject(ctx);

	while (true) {
		token = Json_ConsumeToken(ctx);
		if (token == ',') continue;
		if (token == '}') return;

		if (token != '"') { ctx->failed = true; return; }
		String_InitArray(ctx->curKey, keyBuffer);
		Json_ConsumeString(ctx, &ctx->curKey);

		token = Json_ConsumeToken(ctx);
		if (token != ':') { ctx->failed = true; return; }

		token = Json_ConsumeToken(ctx);
		if (token == TOKEN_NONE) { ctx->failed = true; return; }

		value = Json_ConsumeValue(token, ctx);
		ctx->OnValue(ctx, &value);
		ctx->curKey = oldKey;
	}
}

static void Json_ConsumeArray(struct JsonContext* ctx) {
	String value;
	int token;
	ctx->OnNewArray(ctx);

	while (true) {
		token = Json_ConsumeToken(ctx);
		if (token == ',') continue;
		if (token == ']') return;

		if (token == TOKEN_NONE) { ctx->failed = true; return; }
		value = Json_ConsumeValue(token, ctx);
		ctx->OnValue(ctx, &value);
	}
}

static String Json_ConsumeValue(int token, struct JsonContext* ctx) {
	switch (token) {
	case '{': Json_ConsumeObject(ctx); break;
	case '[': Json_ConsumeArray(ctx);  break;
	case '"': Json_ConsumeString(ctx, &ctx->_tmp); return ctx->_tmp;

	case TOKEN_NUM:   return Json_ConsumeNumber(ctx);
	case TOKEN_TRUE:  return strTrue;
	case TOKEN_FALSE: return strFalse;
	case TOKEN_NULL:  break;
	}
	return String_Empty;
}

static void Json_NullOnNew(struct JsonContext* ctx) { }
static void Json_NullOnValue(struct JsonContext* ctx, const String* v) { }
void Json_Init(struct JsonContext* ctx, String* str) {
	ctx->cur    = str->buffer;
	ctx->left   = str->length;
	ctx->failed = false;
	ctx->curKey = String_Empty;

	ctx->OnNewArray  = Json_NullOnNew;
	ctx->OnNewObject = Json_NullOnNew;
	ctx->OnValue     = Json_NullOnValue;
	String_InitArray(ctx->_tmp, ctx->_tmpBuffer);
}

void Json_Parse(struct JsonContext* ctx) {
	int token;
	do {
		token = Json_ConsumeToken(ctx);
		Json_ConsumeValue(token, ctx);
	} while (token != TOKEN_NONE);
}

static void Json_Handle(uint8_t* data, uint32_t len, 
						JsonOnValue onVal, JsonOnNew newArr, JsonOnNew newObj) {
	struct JsonContext ctx;
	/* NOTE: classicube.net uses \u JSON for non ASCII, no need to UTF8 convert characters */
	String str = String_Init((char*)data, len, len);
	Json_Init(&ctx, &str);
	
	if (onVal)  ctx.OnValue     = onVal;
	if (newArr) ctx.OnNewArray  = newArr;
	if (newObj) ctx.OnNewObject = newObj;
	Json_Parse(&ctx);
}


/*########################################################################################################################*
*--------------------------------------------------------Web task---------------------------------------------------------*
*#########################################################################################################################*/
static void LWebTask_Reset(struct LWebTask* task) {
	task->Completed = false;
	task->Working   = true;
	task->Success   = false;
	task->Start     = DateTime_CurrentUTC_MS();
	task->Res       = 0;
	task->Status    = 0;
}

void LWebTask_Tick(struct LWebTask* task) {
	struct HttpRequest req;
	int delta;
	if (task->Completed) return;

	if (!Http_GetResult(&task->Identifier, &req)) return;
	delta = (int)(DateTime_CurrentUTC_MS() - task->Start);
	Platform_Log2("%s took %i", &task->Identifier, &delta);

	task->Res    = req.Result;
	task->Status = req.StatusCode;

	task->Working   = false;
	task->Completed = true;
	task->Success   = req.Success;
	if (task->Success) task->Handle((uint8_t*)req.Data, req.Size);
	HttpRequest_Free(&req);
}

void LWebTask_DisplayError(struct LWebTask* task, const char* action, String* dst) {
	Launcher_DisplayHttpError(task->Res, task->Status, action, dst);
}


/*########################################################################################################################*
*-------------------------------------------------------GetTokenTask------------------------------------------------------*
*#########################################################################################################################*/
static struct EntryList ccCookies;
struct GetTokenTaskData GetTokenTask;

static void GetTokenTask_OnValue(struct JsonContext* ctx, const String* str) {
	if (!String_CaselessEqualsConst(&ctx->curKey, "token")) return;
	String_Copy(&GetTokenTask.Token, str);
}

static void GetTokenTask_Handle(uint8_t* data, uint32_t len) {
	Json_Handle(data, len, GetTokenTask_OnValue, NULL, NULL);
}

void GetTokenTask_Run(void) {
	static const String id  = String_FromConst("CC get token");
	static const String url = String_FromConst("https://www.classicube.net/api/login");
	static char tokenBuffer[STRING_SIZE];
	if (GetTokenTask.Base.Working) return;

	LWebTask_Reset(&GetTokenTask.Base);
	String_InitArray(GetTokenTask.Token, tokenBuffer);

	GetTokenTask.Base.Identifier = id;
	Http_AsyncGetDataEx(&url, false, &id, NULL, NULL, &ccCookies);
	GetTokenTask.Base.Handle     = GetTokenTask_Handle;
}


/*########################################################################################################################*
*--------------------------------------------------------SignInTask-------------------------------------------------------*
*#########################################################################################################################*/
struct SignInTaskData SignInTask;
char userBuffer[STRING_SIZE];

static void SignInTask_LogError(const String* str) {
	if (String_CaselessEqualsConst(str, "username") || String_CaselessEqualsConst(str, "password")) {
		SignInTask.Error = "&cWrong username or password";
	} else if (String_CaselessEqualsConst(str, "verification")) {
		SignInTask.Error = "&cAccount verification required";
	} else if (str->length) {
		SignInTask.Error = "&cUnknown error occurred";
	}
}

static void SignInTask_OnValue(struct JsonContext* ctx, const String* str) {
	if (String_CaselessEqualsConst(&ctx->curKey, "username")) {
		String_Copy(&SignInTask.Username, str);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "errors")) {
		SignInTask_LogError(str);
	}
}

static void SignInTask_Handle(uint8_t* data, uint32_t len) {
	Json_Handle(data, len, SignInTask_OnValue, NULL, NULL);
}

static void SignInTask_Append(String* dst, const char* key, const String* value) {
	String_AppendConst(dst, key);
	Http_UrlEncodeUtf8(dst, value);
}

void SignInTask_Run(const String* user, const String* pass) {
	static const String id  = String_FromConst("CC post login");
	static const String url = String_FromConst("https://www.classicube.net/api/login");
	String tmp; char tmpBuffer[384];
	if (SignInTask.Base.Working) return;

	LWebTask_Reset(&SignInTask.Base);
	String_InitArray(SignInTask.Username, userBuffer);
	SignInTask.Error = NULL;

	String_InitArray(tmp, tmpBuffer);
	SignInTask_Append(&tmp, "username=",  user);
	SignInTask_Append(&tmp, "&password=", pass);
	SignInTask_Append(&tmp, "&token=",    &GetTokenTask.Token);

	SignInTask.Base.Identifier = id;
	Http_AsyncPostData(&url, false, &id, tmp.buffer, tmp.length, &ccCookies);
	SignInTask.Base.Handle     = SignInTask_Handle;
}


/*########################################################################################################################*
*-----------------------------------------------------FetchServerTask-----------------------------------------------------*
*#########################################################################################################################*/
struct FetchServerData FetchServerTask;
static struct ServerInfo* curServer;

static void ServerInfo_Init(struct ServerInfo* info) {
	String_InitArray(info->hash, info->_hashBuffer);
	String_InitArray(info->name, info->_nameBuffer);
	String_InitArray(info->ip,   info->_ipBuffer);
	String_InitArray(info->mppass,   info->_mppassBuffer);
	String_InitArray(info->software, info->_softBuffer);

	info->players    = 0;
	info->maxPlayers = 0;
	info->uptime     = 0;
	info->featured   = false;
	info->country[0] = 't';
	info->country[1] = '1'; /* 'T1' for unrecognised country */
	info->_order     = -100000;
}

static void ServerInfo_Parse(struct JsonContext* ctx, const String* val) {
	struct ServerInfo* info = curServer;
	if (String_CaselessEqualsConst(&ctx->curKey, "hash")) {
		String_Copy(&info->hash, val);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "name")) {
		String_Copy(&info->name, val);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "players")) {
		Convert_ParseInt(val, &info->players);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "maxplayers")) {
		Convert_ParseInt(val, &info->maxPlayers);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "uptime")) {
		Convert_ParseInt(val, &info->uptime);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "mppass")) {
		String_Copy(&info->mppass, val);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "ip")) {
		String_Copy(&info->ip, val);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "port")) {
		Convert_ParseInt(val, &info->port);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "software")) {
		String_Copy(&info->software, val);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "featured")) {
		Convert_ParseBool(val, &info->featured);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "country_abbr")) {
		/* Two letter country codes, see ISO 3166-1 alpha-2 */
		if (val->length < 2) return;

		/* classicube.net only works with lowercase flag urls */
		info->country[0] = val->buffer[0]; Char_MakeLower(info->country[0]);
		info->country[1] = val->buffer[1]; Char_MakeLower(info->country[1]);
	}
}

static void FetchServerTask_Handle(uint8_t* data, uint32_t len) {
	curServer = &FetchServerTask.Server;
	Json_Handle(data, len, ServerInfo_Parse, NULL, NULL);
}

void FetchServerTask_Run(const String* hash) {
	static const String id  = String_FromConst("CC fetch server");
	String url; char urlBuffer[URL_MAX_SIZE];
	if (FetchServerTask.Base.Working) return;

	LWebTask_Reset(&FetchServerTask.Base);
	ServerInfo_Init(&FetchServerTask.Server);
	String_InitArray(url, urlBuffer);
	String_Format1(&url, "https://www.classicube.net/api/server/%s", hash);

	FetchServerTask.Base.Identifier = id;
	Http_AsyncGetDataEx(&url, false, &id, NULL, NULL, &ccCookies);
	FetchServerTask.Base.Handle  = FetchServerTask_Handle;
}


/*########################################################################################################################*
*-----------------------------------------------------FetchServersTask----------------------------------------------------*
*#########################################################################################################################*/
struct FetchServersData FetchServersTask;
static void FetchServersTask_Count(struct JsonContext* ctx) {
	FetchServersTask.NumServers++;
}

static void FetchServersTask_Next(struct JsonContext* ctx) {
	curServer++;
	if (curServer < FetchServersTask.Servers) return;
	ServerInfo_Init(curServer);
}

static void FetchServersTask_Handle(uint8_t* data, uint32_t len) {
	int count;
	Mem_Free(FetchServersTask.Servers);
	Mem_Free(FetchServersTask.Orders);

	FetchServersTask.NumServers = 0;
	FetchServersTask.Servers    = NULL;
	FetchServersTask.Orders     = NULL;

	/* -1 because servers is surrounded by a { */
	FetchServersTask.NumServers = -1;
	Json_Handle(data, len, NULL, NULL, FetchServersTask_Count);
	count = FetchServersTask.NumServers;

	if (count <= 0) return;
	FetchServersTask.Servers = (struct ServerInfo*)Mem_Alloc(count, sizeof(struct ServerInfo), "servers list");
	FetchServersTask.Orders  = (uint16_t*)Mem_Alloc(count, 2, "servers order");

	/* -2 because servers is surrounded by a { */
	curServer = FetchServersTask.Servers - 2;
	Json_Handle(data, len, ServerInfo_Parse, NULL, FetchServersTask_Next);
}

void FetchServersTask_Run(void) {
	static const String id  = String_FromConst("CC fetch servers");
	static const String url = String_FromConst("https://www.classicube.net/api/servers");
	if (FetchServersTask.Base.Working) return;
	LWebTask_Reset(&FetchServersTask.Base);

	FetchServersTask.Base.Identifier = id;
	Http_AsyncGetDataEx(&url, false, &id, NULL, NULL, &ccCookies);
	FetchServersTask.Base.Handle = FetchServersTask_Handle;
}

void FetchServersTask_ResetOrder(void) {
	int i;
	for (i = 0; i < FetchServersTask.NumServers; i++) {
		FetchServersTask.Orders[i] = i;
	}
}


/*########################################################################################################################*
*-----------------------------------------------------CheckUpdateTask-----------------------------------------------------*
*#########################################################################################################################*/
struct CheckUpdateData CheckUpdateTask;
static char relVersionBuffer[16];

CC_NOINLINE static TimeMS CheckUpdateTask_ParseTime(const String* str) {
	String time, fractional;
	TimeMS ms;
	/* timestamp is in form of "seconds.fractional" */
	/* But only need to care about the seconds here */
	String_UNSAFE_Separate(str, '.', &time, &fractional);

	Convert_ParseUInt64(&time, &ms);
	if (!ms) return 0;
	return ms * 1000 + UNIX_EPOCH;
}

static void CheckUpdateTask_OnValue(struct JsonContext* ctx, const String* str) {
	if (String_CaselessEqualsConst(&ctx->curKey, "release_version")) {
		String_Copy(&CheckUpdateTask.LatestRelease, str);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "latest_ts")) {
		CheckUpdateTask.DevTimestamp = CheckUpdateTask_ParseTime(str);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "release_ts")) {
		CheckUpdateTask.RelTimestamp = CheckUpdateTask_ParseTime(str);
	}
}

static void CheckUpdateTask_Handle(uint8_t* data, uint32_t len) {
	Json_Handle(data, len, CheckUpdateTask_OnValue, NULL, NULL);
}

void CheckUpdateTask_Run(void) {
	static const String id  = String_FromConst("CC update check");
	static const String url = String_FromConst("http://cs.classicube.net/c_client/builds.json");
	if (CheckUpdateTask.Base.Working) return;

	LWebTask_Reset(&CheckUpdateTask.Base);
	CheckUpdateTask.DevTimestamp = 0;
	CheckUpdateTask.RelTimestamp = 0;
	String_InitArray(CheckUpdateTask.LatestRelease, relVersionBuffer);

	CheckUpdateTask.Base.Identifier = id;
	Http_AsyncGetData(&url, false, &id);
	CheckUpdateTask.Base.Handle     = CheckUpdateTask_Handle;
}


/*########################################################################################################################*
*-----------------------------------------------------FetchUpdateTask-----------------------------------------------------*
*#########################################################################################################################*/
struct FetchUpdateData FetchUpdateTask;
static void FetchUpdateTask_Handle(uint8_t* data, uint32_t len) {
	static const String path = String_FromConst("ClassiCube.update");
	ReturnCode res;

	res = Stream_WriteAllTo(&path, data, len);
	if (res) { Logger_Warn(res, "saving update"); return; }

	res = File_SetModifiedTime(&path, FetchUpdateTask.Timestamp);
	if (res) Logger_Warn(res, "setting update time");

	res = File_MarkExecutable(&path);
	if (res) Logger_Warn(res, "making update executable");
}

void FetchUpdateTask_Run(bool release, bool d3d9) {
#if defined CC_BUILD_WIN
#if _WIN64
	const char* exe_d3d9 = "ClassiCube.64.exe";
	const char* exe_ogl  = "ClassiCube.64-opengl.exe";
#else
	const char* exe_d3d9 = "ClassiCube.exe";
	const char* exe_ogl  = "ClassiCube.opengl.exe";
#endif
#elif defined CC_BUILD_LINUX
#if __i386__
	const char* exe_d3d9 = "ClassiCube.32";
	const char* exe_ogl  = "ClassiCube.32";
#elif __x86_64__
	const char* exe_d3d9 = "ClassiCube";
	const char* exe_ogl  = "ClassiCube";
#else
	const char* exe_d3d9 = "ClassiCube.unknown";
	const char* exe_ogl  = "ClassiCube.unknown";
#endif
#elif defined CC_BUILD_OSX
	const char* exe_d3d9 = "ClassiCube.osx";
	const char* exe_ogl  = "ClassiCube.osx";
#else
	const char* exe_d3d9 = "ClassiCube.unknown";
	const char* exe_ogl  = "ClassiCube.unknown";
#endif

	static char idBuffer[24];
	static int idCounter;
	String url; char urlBuffer[URL_MAX_SIZE];
	String_InitArray(url, urlBuffer);

	String_InitArray(FetchUpdateTask.Base.Identifier, idBuffer);
	String_Format1(&FetchUpdateTask.Base.Identifier, "CC update fetch%i", &idCounter);
	/* User may click another update button in the updates menu before original update finished downloading */
	/* Hence must use a different ID for each update fetch, otherwise old update gets downloaded and applied */
	idCounter++;

	String_Format2(&url, "http://cs.classicube.net/c_client/%c/%c",
		release ? "release" : "latest",
		d3d9    ? exe_d3d9  : exe_ogl);
	if (FetchUpdateTask.Base.Working) return;

	LWebTask_Reset(&FetchUpdateTask.Base);
	FetchUpdateTask.Timestamp = release ? CheckUpdateTask.RelTimestamp : CheckUpdateTask.DevTimestamp;

	Http_AsyncGetData(&url, false, &FetchUpdateTask.Base.Identifier);
	FetchUpdateTask.Base.Handle = FetchUpdateTask_Handle;
}


/*########################################################################################################################*
*-----------------------------------------------------FetchFlagsTask-----------------------------------------------------*
*#########################################################################################################################*/
struct FetchFlagsData FetchFlagsTask;
static int flagsCount, flagsCapacity;

struct Flag {
	Bitmap bmp;
	char country[2];
};
static struct Flag* flags;

static void FetchFlagsTask_DownloadNext(void);
static void FetchFlagsTask_Handle(uint8_t* data, uint32_t len) {
	struct Stream s;
	ReturnCode res;

	Stream_ReadonlyMemory(&s, data, len);
	res = Png_Decode(&flags[FetchFlagsTask.Count].bmp, &s);
	if (res) Logger_Warn(res, "decoding flag");

	FetchFlagsTask.Count++;
	FetchFlagsTask_DownloadNext();
}

static void FetchFlagsTask_DownloadNext(void) {
	static const String id = String_FromConst("CC get flag");
	String url; char urlBuffer[URL_MAX_SIZE];
	String_InitArray(url, urlBuffer);

	if (FetchFlagsTask.Base.Working)        return;
	if (FetchFlagsTask.Count == flagsCount) return;

	LWebTask_Reset(&FetchFlagsTask.Base);
	String_Format2(&url, "http://static.classicube.net/img/flags/%r%r.png",
			&flags[FetchFlagsTask.Count].country[0], &flags[FetchFlagsTask.Count].country[1]);

	FetchFlagsTask.Base.Identifier = id;
	Http_AsyncGetData(&url, false, &id);
	FetchFlagsTask.Base.Handle = FetchFlagsTask_Handle;
}

static void FetchFlagsTask_Ensure(void) {
	if (flagsCount < flagsCapacity) return;
	flagsCapacity = flagsCount + 10;

	if (flags) {
		flags = (struct Flag*)Mem_Realloc(flags, flagsCapacity, sizeof(struct Flag), "flags");
	} else {
		flags = (struct Flag*)Mem_Alloc(flagsCapacity,          sizeof(struct Flag), "flags");
	}
}

void FetchFlagsTask_Add(const struct ServerInfo* server) {
	int i;
	for (i = 0; i < flagsCount; i++) {
		if (flags[i].country[0] != server->country[0]) continue;
		if (flags[i].country[1] != server->country[1]) continue;
		/* flag is already or will be downloaded */
		return;
	}
	FetchFlagsTask_Ensure();

	Bitmap_Init(flags[flagsCount].bmp, 0, 0, NULL);
	flags[flagsCount].country[0] = server->country[0];
	flags[flagsCount].country[1] = server->country[1];

	flagsCount++;
	FetchFlagsTask_DownloadNext();
}

Bitmap* Flags_Get(const struct ServerInfo* server) {
	int i;
	for (i = 0; i < FetchFlagsTask.Count; i++) {
		if (flags[i].country[0] != server->country[0]) continue;
		if (flags[i].country[1] != server->country[1]) continue;
		return &flags[i].bmp;
	}
	return NULL;
}

void Flags_Free(void) {
	int i;
	for (i = 0; i < FetchFlagsTask.Count; i++) {
		Mem_Free(flags[i].bmp.Scan0);
	}
}
#endif
