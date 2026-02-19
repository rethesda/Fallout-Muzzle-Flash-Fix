#include "nvse/PluginAPI.h"

void __forceinline SafeWrite32(UInt32 addr, UInt32 data)
{
	UInt32	oldProtect;

	VirtualProtect((void*)addr, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((UInt32*)addr) = data;
	VirtualProtect((void*)addr, 4, oldProtect, &oldProtect);
}

void __forceinline ReplaceCall(UInt32 jumpSrc, UInt32 jumpTgt)
{
	SafeWrite32(jumpSrc + 1, jumpTgt - jumpSrc - 1 - 4);
}

template <typename C, typename Ret, typename... Args>
inline void __fastcall ReplaceCallEx(SIZE_T source, Ret(C::* const target)(Args...) const) {
	union {
		Ret(C::* tgt)(Args...) const;
		SIZE_T funcPtr;
	} conversion;
	conversion.tgt = target;

	ReplaceCall(source, conversion.funcPtr);
}

template <typename C, typename Ret, typename... Args>
inline void __fastcall ReplaceCallEx(SIZE_T source, Ret(C::* const target)(Args...)) {
	union {
		Ret(C::* tgt)(Args...);
		SIZE_T funcPtr;
	} conversion;
	conversion.tgt = target;

	ReplaceCall(source, conversion.funcPtr);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret ThisCall(UInt32 _addr, const void* _this, Args ...args)
{
	return ((T_Ret(__thiscall*)(const void*, Args...))_addr)(_this, std::forward<Args>(args)...);
}

class MuzzleFlash {
	struct NiLight {
		DWORD	pad[12];
		bool	bCulled : 1;
	};

	bool		bEnabled;
	DWORD		pad[3];
	NiLight*	pLight;
public:
	void UpdateLight() {
		if (pLight) [[likely]] 
			pLight->bCulled = !bEnabled;
		ThisCall(0x9BB8A0, this);
	}
};

extern "C" __declspec(dllexport) bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "MuzzleFlashFix";
	info->version = 2;

	return !nvse->isEditor;
}

extern "C" __declspec(dllexport) bool NVSEPlugin_Load(NVSEInterface* nvse) {
	ReplaceCallEx(0x9BB158, &MuzzleFlash::UpdateLight);
	return true;
}