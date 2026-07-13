#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string_view>

class Memory
{
private:
	std::uintptr_t processId = 0;
	void* processHandle = nullptr;

public:
	// Constructor: Changed argument to std::wstring_view to match Unicode
	Memory(const std::wstring_view processName) noexcept
	{
		::PROCESSENTRY32W entry = { }; // Explicitly use the Wide version
		entry.dwSize = sizeof(::PROCESSENTRY32W);

		const auto snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		if (snapShot != INVALID_HANDLE_VALUE) {
			while (::Process32NextW(snapShot, &entry))
			{
				// Now both are wide strings, so they can be compared properly
				if (!processName.compare(entry.szExeFile))
				{
					processId = entry.th32ProcessID;
					processHandle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
					break;
				}
			}
			::CloseHandle(snapShot);
		}
	}

	~Memory()
	{
		if (processHandle)
			::CloseHandle(processHandle);
	}

	// Returns the base address: Changed argument to std::wstring_view
	const std::uintptr_t GetModuleAddress(const std::wstring_view moduleName) const noexcept
	{
		::MODULEENTRY32W entry = { }; // Explicitly use the Wide version
		entry.dwSize = sizeof(::MODULEENTRY32W);

		const auto snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
		std::uintptr_t result = 0;

		if (snapShot != INVALID_HANDLE_VALUE) {
			while (::Module32NextW(snapShot, &entry))
			{
				if (!moduleName.compare(entry.szModule))
				{
					result = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
					break;
				}
			}
			::CloseHandle(snapShot);
		}

		return result;
	}

	template <typename T>
	constexpr const T Read(const std::uintptr_t& address) const noexcept
	{
		T value = { };
		::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), &value, sizeof(T), NULL);
		return value;
	}

	template <typename T>
	constexpr void Write(const std::uintptr_t& address, const T& value) const noexcept
	{
		::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), &value, sizeof(T), NULL);
	}
};