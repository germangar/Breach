#include "utils.h"

namespace TestConstObject
{

#define TESTNAME "TestConstObject"

static const char *script2 = 
"const string URL_SITE = \"http://www.sharkbaitgames.com\";                    \n"
"const string URL_GET_HISCORES = URL_SITE + \"/get_hiscores.php\";             \n"
"const string URL_CAN_SUBMIT_HISCORE = URL_SITE + \"/can_submit_hiscore.php\"; \n"
"const string URL_SUBMIT_HISCORE = URL_SITE + \"/submit_hiscore.php\";         \n";

static const char *script = 
"void Test(obj@ o) { }";

static const char *script3 =
"class CTest                           \n"
"{                                     \n"
"	int m_Int;                         \n"
"                                      \n"
"	void SetInt(int iInt)              \n"
"	{                                  \n"
"		m_Int = iInt;                  \n"
"	}                                  \n"
"};                                    \n"
"void func()                           \n"
"{                                     \n"
"	const CTest Test;                  \n"
"	const CTest@ TestHandle = @Test;   \n"
"                                      \n"
"	TestHandle.SetInt(1);              \n"    
"	Test.SetInt(1);                    \n"          
"}                                     \n";

class CObj
{
public: 
	CObj() {refCount = 1; val = 0; next = 0;}
	~CObj() {if( next ) next->Release();}

	CObj &operator=(CObj &other) 
	{
		val = other.val; 
		if( next ) 
			next->Release(); 
		next = other.next; 
		if( next ) 
			next->AddRef();
		return *this; 
	};

	void AddRef() {refCount++;}
	void Release() {if( --refCount == 0 ) delete this;}

	void SetVal(int val) {this->val = val;}
	int GetVal() const {return val;}

	int &operator[](int) {return val;}
	const int &operator[](int) const {return val;}

	int refCount;

	int val;

	CObj *next;
};

CObj *CObj_Factory()
{
	return new CObj();
}

CObj c_obj;

bool Test2();

bool Test()
{
	bool fail = Test2();
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		return false;

	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	// Register an object type
	r = engine->RegisterObjectType("obj", sizeof(CObj), asOBJ_REF); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_FACTORY, "obj@ f()", asFUNCTION(CObj_Factory), asCALL_CDECL); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_ADDREF, "void f()", asMETHOD(CObj,AddRef), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_RELEASE, "void f()", asMETHOD(CObj,Release), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_ASSIGNMENT, "obj &f(const obj &in)", asMETHOD(CObj,operator=), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_INDEX, "int &f(int)", asMETHODPR(CObj, operator[], (int), int&), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_INDEX, "const int &f(int) const", asMETHODPR(CObj, operator[], (int) const, const int&), asCALL_THISCALL); assert( r>=0 );

	r = engine->RegisterObjectType("prop", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r>=0 );
	r = engine->RegisterObjectProperty("prop", "int val", 0); assert( r>=0 );

	r = engine->RegisterObjectMethod("obj", "void SetVal(int)", asMETHOD(CObj, SetVal), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectMethod("obj", "int GetVal() const", asMETHOD(CObj, GetVal), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectProperty("obj", "int val", offsetof(CObj,val)); assert( r>=0 );
	r = engine->RegisterObjectProperty("obj", "obj@ next", offsetof(CObj,next)); assert( r>=0 );
	r = engine->RegisterObjectProperty("obj", "prop p", offsetof(CObj,val)); assert( r>=0 );

	r = engine->RegisterGlobalProperty("const obj c_obj", &c_obj); assert( r>=0 );
	r = engine->RegisterGlobalProperty("obj g_obj", &c_obj); assert( r>= 0 );

	RegisterScriptString(engine);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script1", script2, strlen(script2), 0);
	r = mod->Build();
	if( r < 0 ) fail = true;


	CBufferedOutStream bout;

	// TODO: A member array of a const object is also const

	// TODO: Parameters registered as &in and not const must make a copy of the object (even for operators)

	// A member object of a const object is also const
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "c_obj.p.val = 1;");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 13) : Error   : Reference is read-only\n" ) fail = true;

	c_obj.val = 0;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->ExecuteString(0, "g_obj.p.val = 1;");
	if( r < 0 ) fail = true;
	if( c_obj.val != 1 ) fail = true;

	// Allow overloading on const.
	r = engine->ExecuteString(0, "obj o; o[0] = 1;");
	if( r < 0 ) fail = true;

	// Allow return of const ref
	r = engine->ExecuteString(0, "int a = c_obj[0];");
	if( r < 0 ) fail = true;

	// Do not allow the script to call object behaviour that is not const on a const object
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "c_obj[0] = 1;");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 10) : Error   : Reference is read-only\n" ) fail = true;

	// Do not allow the script to take a non-const handle to a const object
	bout.buffer = "";
	r = engine->ExecuteString(0, "obj@ o = @c_obj;");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 10) : Error   : Can't implicitly convert from 'const obj@' to 'obj@&'.\n" )
		fail = true;

	// Do not allow the script to pass a const obj@ to a parameter that is not a const obj@
	mod->AddScriptSection("script", script, strlen(script));
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	mod->Build();
	
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "Test(@c_obj);");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 1) : Error   : No matching signatures to 'Test(const obj@const&)'\n" )
		fail = true;

	// Do not allow the script to assign the object handle member of a const object
	bout.buffer = "";
	r = engine->ExecuteString(0, "@c_obj.next = @obj();");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 13) : Error   : Reference is read-only\n" )
		fail = true;

	// Allow the script to change the object the handle points to
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->ExecuteString(0, "c_obj.next.val = 1;");
	if( r != 3 ) fail = true;

	// Allow the script take a handle to a non const object handle in a const object
	r = engine->ExecuteString(0, "obj @a = @c_obj.next;");
	if( r < 0 ) fail = true;

	// Allow the script to take a const handle to a const object
	r = engine->ExecuteString(0, "const obj@ o = @c_obj;");
	if( r < 0 ) fail = true;

	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "obj @a; const obj @b; @a = @b;");
	if( r >= 0 ) fail = true;
	if(bout.buffer != "ExecuteString (1, 28) : Error   : Can't implicitly convert from 'const obj@&' to 'obj@&'.\n" )
		fail = true;

	// Allow a non-const handle to be assigned to a const handle
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->ExecuteString(0, "obj @a; const obj @b; @b = @a;");
	if( r < 0 ) fail = true;

	// Do not allow the script to alter properties of a const object
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "c_obj.val = 1;");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 11) : Error   : Reference is read-only\n" )
		fail = true;

	// Do not allow the script to call non-const methods on a const object
	bout.buffer = "";
	r = engine->ExecuteString(0, "c_obj.SetVal(1);");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 7) : Error   : No matching signatures to 'obj::SetVal(const uint) const'\n" )
		fail = true;

	// Allow the script to call const methods on a const object
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->ExecuteString(0, "c_obj.GetVal();");
	if( r < 0 ) fail = true;

	// Handle to const must not allow call to non-const methods
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	mod->AddScriptSection("script", script3, strlen(script3));
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (10, 1) : Info    : Compiling void func()\n"
		               "script (15, 13) : Error   : No matching signatures to 'CTest::SetInt(const uint) const'\n"
					   "script (16, 7) : Error   : No matching signatures to 'CTest::SetInt(const uint) const'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Allow passing a const object to a function that takes a non-const object by value
	bout.buffer = "";
	const char *script4 = "void func(prop val) {}";
	mod->AddScriptSection("script", script4, strlen(script4));
	r = mod->Build();
	if( r < 0 ) 
		fail = true;
	r = engine->ExecuteString(0, "const prop val; func(val)");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	if( fail )
		printf("%s: failed\n", TESTNAME);

	// Success
	return fail;
}



bool Test2()
{
	bool fail = false;

	int r;
	COutStream out;
	CBufferedOutStream bout;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	// Try calling a const method on a const object (success)
	const char *script = 
	"class MyType                  \n"
	"{                             \n"
	"   int val;                   \n"
	"   void TestConst() const     \n"
	"   {                          \n"
	"      assert(val == 5);       \n"
	"   }                          \n"
	"}                             \n"
	"void Func(const MyType &in a) \n"
	"{                             \n"
	"   a.TestConst();             \n"
	"}                             \n";

	asIScriptModule *mod = engine->GetModule("module", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	// Try calling a non-const method on a const object (should fail)
	script =
	"class MyType                  \n"
	"{                             \n"
	"   int val;                   \n"
	"   void TestConst()           \n"
	"   {                          \n"
	"      assert(val == 5);       \n"
	"   }                          \n"
	"}                             \n"
	"void Func(const MyType &in a) \n"
	"{                             \n"
	"   a.TestConst();             \n"
	"}                             \n";

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

	bout.buffer = "";
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (9, 1) : Info    : Compiling void Func(const MyType&in)\n"
					   "script (11, 6) : Error   : No matching signatures to 'MyType::TestConst() const'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Try modifying self in a const method (should fail)
	script = 
	"class MyType                  \n"
	"{                             \n"
	"   int val;                   \n"
	"   void TestConst() const     \n"
	"   {                          \n"
	"      val = 5;                \n"
	"   }                          \n"
	"}                             \n";
	
	bout.buffer = "";
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (4, 4) : Info    : Compiling void MyType::TestConst() const\n"
					   "script (6, 11) : Error   : Reference is read-only\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Try modifying self via 'this' in a const method (should fail)
	script = 
	"class MyType                  \n"
	"{                             \n"
	"   int val;                   \n"
	"   void TestConst() const     \n"
	"   {                          \n"
	"      this.val = 5;           \n"
	"   }                          \n"
	"}                             \n";
	
	bout.buffer = "";
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (4, 4) : Info    : Compiling void MyType::TestConst() const\n"
					   "script (6, 16) : Error   : Reference is read-only\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	return fail;
}

} // namespace

