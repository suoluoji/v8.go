#include <stdlib.h>
#include <stdint.h>
#include "v8.h"

extern "C" {

using namespace v8;

class V8_Context {
public:
	V8_Context(Isolate* isolate, Handle<Context> context) {
		isolate_ = isolate;
		self.Reset(isolate, context);
	}

	~V8_Context() {
		Locker locker(isolate_);
		Isolate::Scope isolate_scope(isolate_);

		self.Dispose();
		self.Reset();
	}

	Isolate* GetIsolate() {
		return isolate_;
	}

	Isolate* isolate_;
	Persistent<Context> self;
};

class V8_Script {
public:
	V8_Script(Isolate* isolate, Handle<Script> script) {
		isolate_ = isolate;
		self.Reset(isolate, script);
	}

	~V8_Script() {
		Locker locker(isolate_);
		Isolate::Scope isolate_scope(isolate_);

		self.Dispose();
		self.Reset();
	}

	Isolate* GetIsolate() {
		return isolate_;
	}

	Isolate* isolate_;
	Persistent<Script> self;
};

class V8_Value {
public:
	V8_Value(Isolate* isolate, Handle<Context> context_handle, Handle<Value> value) {
		isolate_ = isolate;
		self.Reset(isolate, value);
		context.Reset(isolate, context_handle);
	}

	~V8_Value() {
		Locker locker(isolate_);
		Isolate::Scope isolate_scope(isolate_);

		self.Dispose();
		self.Reset();
		context.Dispose();
		context.Reset();
	}

	Isolate* GetIsolate() {
		return isolate_;
	}

	Isolate* isolate_;
	Persistent<Value> self;
	Persistent<Context> context;
};

#define ISOLATE_SCOPE(isolate_ptr) \
	Isolate* isolate = isolate_ptr; \
	Locker locker(isolate); \
	Isolate::Scope isolate_scope(isolate); \
	HandleScope handle_scope(isolate) \

#define VALUE_TO_LOCAL(value, local_value) \
	V8_Value* val = static_cast<V8_Value*>(value); \
	ISOLATE_SCOPE(val->GetIsolate()); \
	Local<Value> local_value = Local<Value>::New(isolate, val->self)

#define VALUE_SCOPE(value, local_value) \
	V8_Value* val = static_cast<V8_Value*>(value); \
	ISOLATE_SCOPE(val->GetIsolate()); \
	Local<Context> context = Local<Context>::New(isolate, val->context); \
	Context::Scope context_scope(context); \
	Local<Value> local_value = Local<Value>::New(isolate, val->self) \

/*
isolate wrappers
*/
void* V8_NewEngine() {
	ISOLATE_SCOPE(Isolate::New());

	Handle<Context> context = Context::New(isolate);

	if (context.IsEmpty())
		return NULL;

	return (void*)(new V8_Context(isolate, context));
}

void V8_DisposeEngine(void* engine) {
	V8_Context* ctx = static_cast<V8_Context*>(engine);
	Isolate* isolate = ctx->GetIsolate();
	delete ctx;
	isolate->Dispose();
}

/*
context wrappers
*/
void* V8_NewContext(void* engine) {
	V8_Context* ctx = static_cast<V8_Context*>(engine);

	ISOLATE_SCOPE(ctx->GetIsolate());
	
	Handle<Context> context = Context::New(isolate);

	if (context.IsEmpty())
		return NULL;

	return (void*)(new V8_Context(isolate, context));
}

void V8_DisposeContext(void* context) {
	delete static_cast<V8_Context*>(context);
}

/*
script wrappers
*/
void* V8_Compile(void* engine, const char* code, int length, void* script_origin,void* script_data) {
	V8_Context* ctx = static_cast<V8_Context*>(engine);

	ISOLATE_SCOPE(ctx->GetIsolate());

	Local<Context> local_context = Local<Context>::New(isolate, ctx->self);

	Context::Scope context_scope(local_context);

	Handle<Script> script = Script::New(
		String::NewFromOneByte(isolate, (uint8_t*)code, String::kNormalString, length), 
		static_cast<ScriptOrigin*>(script_origin), 
		static_cast<ScriptData*>(script_data),
		Handle<String>()
	);

	if (script.IsEmpty())
		return NULL;

	return (void*)(new V8_Script(isolate, script));
}

void V8_DisposeScript(void* script) {
	delete static_cast<V8_Script*>(script);
}

void* V8_RunScript(void* context, void* script) {
	V8_Context* ctx = static_cast<V8_Context*>(context);
	V8_Script* spt = static_cast<V8_Script*>(script);

	ISOLATE_SCOPE(ctx->GetIsolate());

	Local<Context> local_context = Local<Context>::New(isolate, ctx->self);
	Local<Script> local_script = Local<Script>::New(isolate, spt->self);

	Context::Scope context_scope(local_context);
	
	Handle<Value> result = local_script->Run();

	if (result.IsEmpty())
		return NULL;

	return (void*)(new V8_Value(isolate, local_context, result));
}

/*
script data wrappers
*/
void* V8_PreCompile(void* engine, const char* code, int length) {
	V8_Context* ctx = static_cast<V8_Context*>(engine);
	ISOLATE_SCOPE(ctx->GetIsolate());

	return (void*)ScriptData::PreCompile(
		String::NewFromOneByte(isolate, (uint8_t*)code, String::kNormalString, length)
	);
}

void* V8_NewScriptData(const char* data, int length) {
	return (void*)ScriptData::New(data, length);
}

void V8_DisposeScriptData(void* script_data) {
	delete static_cast<ScriptData*>(script_data);
}

int V8_ScriptDataLength(void* script_data) {
	return static_cast<ScriptData*>(script_data)->Length();
}

const char* V8_ScriptDataGetData(void* script_data) {
	return static_cast<ScriptData*>(script_data)->Data();
}

int V8_ScriptDataHasError(void* script_data) {
	return static_cast<ScriptData*>(script_data)->HasError();
}

/*
script origin wrappers
*/
void* V8_NewScriptOrigin(void* engine, const char* name, int name_length, int line_offset, int column_offset) {
	V8_Context* ctx = static_cast<V8_Context*>(engine);
	ISOLATE_SCOPE(ctx->GetIsolate());

	return (void*)(new ScriptOrigin(
		String::NewFromOneByte(isolate, (uint8_t*)name, String::kNormalString, name_length),
		Integer::New(line_offset),
		Integer::New(line_offset)
	));
}

void V8_DisposeScriptOrigin(void* script_origin) {
	delete static_cast<ScriptOrigin*>(script_origin);
}

/*
Value wrappers
*/
void V8_DisposeValue(void* value) {
	delete static_cast<V8_Value*>(value);
}

char* V8_ValueToString(void* value) {
	VALUE_TO_LOCAL(value, local_value);

	Handle<String> string = local_value->ToString();
	uint8_t* str = (uint8_t*)malloc(string->Length() + 1);
	string->WriteOneByte(str);
	return (char*)str;
}

int V8_ValueIsUndefined(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsUndefined();
}

int V8_ValueIsNull(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsNull();
}

int V8_ValueIsTrue(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsTrue();
}

int V8_ValueIsFalse(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsFalse();
}

int V8_ValueIsString(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsString();
}

int V8_ValueIsFunction(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsFunction();
}

int V8_ValueIsArray(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsArray();
}

int V8_ValueIsObject(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsObject();
}

int V8_ValueIsBoolean(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsBoolean();
}

int V8_ValueIsNumber(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsNumber();
}

int V8_ValueIsExternal(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsExternal();
}

int V8_ValueIsInt32(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsInt32();
}

int V8_ValueIsUint32(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsUint32();
}

int V8_ValueIsDate(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsDate();
}

int V8_ValueIsBooleanObject(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsBooleanObject();
}

int V8_ValueIsNumberObject(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsNumberObject();
}

int V8_ValueIsStringObject(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsStringObject();
}

int V8_ValueIsNativeError(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsNativeError();
}

int V8_ValueIsRegExp(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IsRegExp();
}

/*
special values
*/
void* V8_Undefined(void* engine) {
	V8_Context* ctx = static_cast<V8_Context*>(engine);
	ISOLATE_SCOPE(ctx->GetIsolate());
	return (void*)(new V8_Value(isolate, Handle<Context>(), Undefined(isolate)));
}

void* V8_Null(void* engine) {
	V8_Context* ctx = static_cast<V8_Context*>(engine);
	ISOLATE_SCOPE(ctx->GetIsolate());
	return (void*)(new V8_Value(isolate, Handle<Context>(), Null(isolate)));
}

void* V8_True(void* engine) {
	V8_Context* ctx = static_cast<V8_Context*>(engine);
	ISOLATE_SCOPE(ctx->GetIsolate());
	return (void*)(new V8_Value(isolate, Handle<Context>(), True(isolate)));
}

void* V8_False(void* engine) {
	V8_Context* ctx = static_cast<V8_Context*>(engine);
	ISOLATE_SCOPE(ctx->GetIsolate());
	return (void*)(new V8_Value(isolate, Handle<Context>(), False(isolate)));
}

int V8_ValueToBoolean(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->BooleanValue();
}
  
double V8_ValueToNumber(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->NumberValue();
}

int64_t V8_ValueToInteger(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->IntegerValue();
}

uint32_t V8_ValueToUint32(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->Uint32Value();
}

int32_t V8_ValueToInt32(void* value) {
	VALUE_TO_LOCAL(value, local_value);
	return local_value->Int32Value();
}

void* V8_NewNumber(void* context, double val) {
	V8_Context* ctx = static_cast<V8_Context*>(context);
	ISOLATE_SCOPE(ctx->GetIsolate());
	Local<Context> local_context = Local<Context>::New(isolate, ctx->self);
	
	return (void*)(new V8_Value(isolate, local_context, 
		Number::New(isolate, val)
	));
}

void* V8_NewString(void* context, const char* val, int val_length) {
	V8_Context* ctx = static_cast<V8_Context*>(context);
	ISOLATE_SCOPE(ctx->GetIsolate());
	Local<Context> local_context = Local<Context>::New(isolate, ctx->self);
	
	return (void*)(new V8_Value(isolate, local_context, 
		String::NewFromOneByte(isolate, (uint8_t*)val, String::kNormalString, val_length)
	));
}

/*
object wrappers
*/

int V8_SetProperty(void* value, const char* key, int key_length, void* prop_value, int attribs) {
	VALUE_SCOPE(value, local_value);

	return Local<Object>::Cast(local_value)->Set(
		String::NewFromOneByte(isolate, (uint8_t*)key, String::kNormalString, key_length),
		Local<Value>::New(isolate, static_cast<V8_Value*>(prop_value)->self),
		(v8::PropertyAttribute)attribs
	);
}

void* V8_GetProperty(void* value, const char* key, int key_length) {
	VALUE_SCOPE(value, local_value);

	return (void*)(new V8_Value(isolate, context,
		Local<Object>::Cast(local_value)->Get(
			String::NewFromOneByte(isolate, (uint8_t*)key, String::kNormalString, key_length)
		)
	));
}

int V8_SetElement(void* value, uint32_t index, void* elem_value) {
	VALUE_SCOPE(value, local_value);

	return Local<Object>::Cast(local_value)->Set(
		index,
		Local<Value>::New(isolate, static_cast<V8_Value*>(elem_value)->self)
	);
}

void* V8_GetElement(void* value, uint32_t index) {
	VALUE_SCOPE(value, local_value);

	return (void*)(new V8_Value(isolate, context,
		Local<Object>::Cast(local_value)->Get(index)
	));
}

int V8_GetPropertyAttributes(void *value, const char* key, int key_length) {
	VALUE_SCOPE(value, local_value);

	return Local<Object>::Cast(local_value)->GetPropertyAttributes(
		String::NewFromOneByte(isolate, (uint8_t*)key, String::kNormalString, key_length)
	);
}

int V8_ForceSetProperty(void* value, const char* key, int key_length, void* prop_value, int attribs) {
	VALUE_SCOPE(value, local_value);

	return Local<Object>::Cast(local_value)->ForceSet(
		String::NewFromOneByte(isolate, (uint8_t*)key, String::kNormalString, key_length),
		Local<Value>::New(isolate, static_cast<V8_Value*>(prop_value)->self),
		(v8::PropertyAttribute)attribs
	);
}

int V8_HasProperty(void *value, const char* key, int key_length) {
	VALUE_SCOPE(value, local_value);

	return Local<Object>::Cast(local_value)->Has(
		String::NewFromOneByte(isolate, (uint8_t*)key, String::kNormalString, key_length)
	);
}

int V8_DeleteProperty(void *value, const char* key, int key_length) {
	VALUE_SCOPE(value, local_value);

	return Local<Object>::Cast(local_value)->Delete(
		String::NewFromOneByte(isolate, (uint8_t*)key, String::kNormalString, key_length)
	);
}

int V8_ForceDeleteProperty(void *value, const char* key, int key_length) {
	VALUE_SCOPE(value, local_value);

	return Local<Object>::Cast(local_value)->ForceDelete(
		String::NewFromOneByte(isolate, (uint8_t*)key, String::kNormalString, key_length)
	);
}

int V8_HasElement(void* value, uint32_t index) {
	VALUE_SCOPE(value, local_value);

	return Local<Object>::Cast(local_value)->Has(index);
}

int V8_DeleteElement(void* value, uint32_t index) {
	VALUE_SCOPE(value, local_value);

	return Local<Object>::Cast(local_value)->Delete(index);
}

int V8_ArrayLength(void* value) {
	VALUE_TO_LOCAL(value, local_value);

	return Local<Array>::Cast(local_value)->Length();
}

} // extern "C"