//
// This test was designed to test the functionality of methods
// from classes with multiple inheritance
//
// Author: Andreas J?nsson
//

#include "utils.h"

#define TESTNAME "TestMultipleInheritance"

static std::string output2;

class CBase1
{
public:
	CBase1() {me1 = "CBase1";}
	virtual void CallMe1()
	{
		output2 += me1;
		output2 += ": ";
		output2 += "CBase1::CallMe1()\n";
	}
	const char *me1;
};

class CBase2
{
public:
	CBase2() {me2 = "CBase2";}
	virtual void CallMe2()
	{
		output2 += me2;
		output2 += ": ";
		output2 += "CBase2::CallMe2()\n";
	}
	const char *me2;
};

class CDerivedMultiple : public CBase1, public CBase2
{
public:
	CDerivedMultiple() : CBase1(), CBase2() {}
};


static CDerivedMultiple d;

bool TestMultipleInheritance2();

bool TestMultipleInheritance()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}
	bool fail = TestMultipleInheritance2();
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	// Register the CDerived class
	r = engine->RegisterObjectType("class1", 0, asOBJ_REF | asOBJ_NOHANDLE);
	r = engine->RegisterObjectMethod("class1", "void CallMe1()", asMETHOD(CDerivedMultiple, CallMe1), asCALL_THISCALL);
	r = engine->RegisterObjectMethod("class1", "void CallMe2()", asMETHOD(CDerivedMultiple, CallMe2), asCALL_THISCALL);

	// Register the global CDerived object
	r = engine->RegisterGlobalProperty("class1 d", &d);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->ExecuteString(0, "d.CallMe1(); d.CallMe2();");

	if( output2 != "CBase1: CBase1::CallMe1()\nCBase2: CBase2::CallMe2()\n" )
	{
		printf("%s: Method calls failed.\n%s", TESTNAME, output2.c_str());
		fail = true;
	}

	engine->Release();

	return fail;
}

//-------------------------------------------------------------------------

// http://www.gamedev.net/community/forums/topic.asp?topic_id=521199


bool addOverlayCalled = false;

class Drawable {
private:
    int somestuff;
protected:
	int somethingElse;

public:
    Drawable(  ) {}
    virtual ~Drawable() {}

    virtual void addOverlay(  ) { addOverlayCalled = true; }
};

class Creep {
private:
	int somestuffOfMine;
protected:
	int duh;
public:
	Creep() {}
	virtual ~Creep() {}

};

class CreepClient : public Creep, public Drawable {
private:
protected:
public:
    CreepClient( ) {}
    virtual ~CreepClient() {}
};

void Dummy() {}


bool Exec(asIScriptEngine *engine, Creep &c)
{
	bool fail = false;

	CreepClient &cc = dynamic_cast<CreepClient&>(c);

	asIScriptModule *mod = engine->GetModule("mod");
	int funcId = mod->GetFunctionIdByIndex(0);
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(funcId);
	ctx->SetArgObject(0, &cc);
	int r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	ctx->Release();

	return fail;

}

bool TestMultipleInheritance2()
{
	bool fail = false;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

    r = engine->RegisterObjectType("CreepClient", sizeof(CreepClient), asOBJ_REF ); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("CreepClient", asBEHAVE_ADDREF, "void f()", asFUNCTION(Dummy), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("CreepClient", asBEHAVE_RELEASE, "void f()", asFUNCTION(Dummy), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

    r = engine->RegisterObjectMethod("CreepClient", "void addOverlay(  )", asMETHOD(CreepClient, addOverlay), asCALL_THISCALL); assert( r >= 0 );

	const char *script =
		"void fireEffects( CreepClient@ c ) { \n"
		"		c.addOverlay(  ); // This one doesn't ever fire off  \n"
		"} \n";

	asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

    CreepClient cc;

	fail = Exec(engine, cc) || fail;

	if( addOverlayCalled == false )
		fail = true;

	engine->Release();

	return fail;
}
