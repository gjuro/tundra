// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_FoundationPlatformNix_h
#define incl_FoundationPlatformNix_h

#if !defined(_WINDOWS)

#include "ConfigAPI.h"
#include <iostream>

namespace Foundation
{
    class Framework;

    /// Low-level *nix specific functionality
    /** \ingroup Foundation_group
    */
    class PlatformNix
    {
    public:
        /// default constructor
        explicit PlatformNix(Framework *framework) : framework_(framework) {}
        /// destructor
        virtual ~PlatformNix() {}

        static void Message(const std::string& title, const std::string& text)
        {
            std::cerr << title << " " << text;
        }

        static void Message(const std::wstring& title, const std::wstring& text)
        {
            std::wcerr << title << " " << text;
        }

        /// Returns current Naali install directory. May be different than the current working directory.
        std::string GetInstallDirectory();
        
        /// Returns current Naali install directory (wide string) May be different than the current working directory.
        std::wstring GetInstallDirectoryW();
        
        /// Returns user specific application data directory.
        /** Returns non-unicode path. May throw Expection if folder is not found.
        */
        std::string GetApplicationDataDirectory();

        /// Returns user specific application data directory.
        /** Returns unicode path. May throw Expection if folder is not found.
        */
        std::wstring GetApplicationDataDirectoryW();

        /// \copydoc Foundation::PlatformWin::GetUserDocumentsDirectory()
        std::string GetUserDocumentsDirectory();

        /// \copydoc PlatformWin::GetUserDocumentsDirectoryW()
        std::wstring GetUserDocumentsDirectoryW();

    private:
        Framework *framework_;
    };
}
#endif

#endif



