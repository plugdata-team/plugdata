//
//  main.c
//  LV2FileMaker
//
//  Created by Guillot Pierre on 6/30/18.
//  Copyright © 2018 Pierre Guillot. All rights reserved.
//

#ifdef _WIN32

#include <stdio.h>
#include <windows.h>

#define LIB_EXT "dll"

//Define the function prototype
typedef void *(t_generate_file)(const char*);

int main(int argc, const char * argv[]) {
   BOOL freeResult, runTimeLinkSuccess = FALSE;
   HINSTANCE dllHandle = NULL;
   t_generate_file *method = NULL;
   if(argc < 3)
   {
       printf("Please provide the path to the LV2 plugin and the name of the plugin\n");
       return 1;
   }
   printf("looking for %s in %s\n", argv[2], argv[1]);
   //Load the dll and keep the handle to it
   dllHandle = LoadLibrary(argv[1]);
   // If the handle is valid, try to get the function address.
   if (NULL != dllHandle)
   {
      //Get pointer to our function using GetProcAddress:
      method = (t_generate_file *)GetProcAddress(dllHandle, "lv2_generate_ttl");

      // If the function address is valid, call the function.
      if (runTimeLinkSuccess = (NULL != method))
      {
         (*method)(argv[2]);
      }

      //Free the library:
      freeResult = FreeLibrary(dllHandle);
   }

   //If unable to call the DLL function, use an alternative.
   if(!runTimeLinkSuccess)
      printf("message via alternative method\n");

   return 0;
}


#else

#include <stdio.h>
#include <dlfcn.h>
#include <libgen.h>
#include <string.h>

#ifdef __APPLE__
#define LIB_EXT "dylib"
#else
#define LIB_EXT "so"
#endif

typedef void *(t_generate_file)(const char*);

int main(int argc, const char * argv[]) {
    if(argc < 3)
    {
        printf("Please provide the path to the LV2 plugin and the name of the plugin\n");
        return 1;
    }
    printf("generating ttl %s in %s\n", argv[2], argv[1]);
    void* handle = dlopen(argv[1], RTLD_LAZY);
    if(handle)
    {
        char* error = NULL;
        t_generate_file* method = (t_generate_file *)dlsym(handle, "lv2_generate_ttl");
        if(method)
        {
            (*method)(argv[2]);
        }
        else
        {
            printf("can't load method lv2_generate_ttl\n");
        }
        if((error = dlerror()) != NULL)
        {
            printf("error: %s", error);
        }
        dlclose(handle);
    }
    else
    {
        char* error = NULL;
        printf("can't open %s\n", argv[1]);
        if((error = dlerror()) != NULL)
        {
            printf("error: %s", error);
        }
    }
    return 0;
}

#endif
