#include <iostream> 
typedef int (*Func)(const char*); 
typedef int (*Func2)(); 
int main() { 
  HMODULE h = LoadLibraryA("SearchBridge64.dll"); 
  if (!h) { std::cout << "DLL not found" << std::endl; return 1; } 
  auto Init = (Func2)GetProcAddress(h, "InitEngine"); 
  auto Load = (Func)GetProcAddress(h, "LoadDocuments"); 
  auto Count = (Func2)GetProcAddress(h, "GetDocCount"); 
  if (!Init || !Load || !Count) { std::cout << "Functions missing" << std::endl; return 1; } 
  Init(); 
  int n = Load("text_data"); 
  std::cout << "Loaded: " << n << " docs" << std::endl; 
  std::cout << "Count: " << Count() << std::endl; 
  FreeLibrary(h); 
  return 0; 
} 
