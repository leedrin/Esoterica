#include "FileSystemWatcher.h"
#include "Base/FileSystem/FileSystem.h"

#include "Base/Platform/PlatformUtils_Win32.h"

//-------------------------------------------------------------------------

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

//-------------------------------------------------------------------------

#if _WIN32
namespace EE::FileSystem
{
    Watcher::Watcher()
    {
        // Create overlapped event
        OVERLAPPED* pOverlappedEvent = EE::New<OVERLAPPED>();
        Memory::MemsetZero( pOverlappedEvent, sizeof( OVERLAPPED ) );
        m_pOverlappedEvent = pOverlappedEvent;
    }

    Watcher::~Watcher()
    {
        StopWatching();

        // Delete overlapped event
        auto pOverlappedEvent = reinterpret_cast<OVERLAPPED*>( m_pOverlappedEvent );
        EE::Delete( pOverlappedEvent );
    }

    bool Watcher::StartWatching( Path const& directoryToWatch )
    {
        EE_ASSERT( !IsWatching() );
        EE_ASSERT( directoryToWatch.IsValid() && directoryToWatch.IsDirectoryPath() );
        m_directoryToWatch = directoryToWatch;

        // Get directory handle
        //-------------------------------------------------------------------------

        m_pDirectoryHandle = CreateFile( directoryToWatch.c_str(), GENERIC_READ | FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            NULL );

        if ( m_pDirectoryHandle == INVALID_HANDLE_VALUE )
        {
            EE_LOG_ERROR( "FileSystem", "File System Watcher", "Failed to open handle to directory (%s), error: %s", m_directoryToWatch.c_str(), Platform::Win32::GetLastErrorMessage().c_str() );
            m_pDirectoryHandle = nullptr;
            return false;
        }

        // Create event
        //-------------------------------------------------------------------------

        auto pOverlappedEvent = reinterpret_cast<OVERLAPPED*>( m_pOverlappedEvent );
        pOverlappedEvent->hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( pOverlappedEvent->hEvent == nullptr )
        {
            EE_LOG_ERROR( "FileSystem", "File System Watcher", "Failed to create overlapped event: %s", Platform::Win32::GetLastErrorMessage().c_str() );
            CloseHandle( m_pDirectoryHandle );
            m_pDirectoryHandle = nullptr;
            return false;
        }

        RequestListOfDirectoryChanges();

        return true;
    }

    void Watcher::StopWatching()
    {
        auto pOverlappedEvent = reinterpret_cast<OVERLAPPED*>( m_pOverlappedEvent );

        // If we are still waiting for an IO request, cancel it
        if ( m_requestPending )
        {
            CancelIo( m_pDirectoryHandle );
            GetOverlappedResult( m_pDirectoryHandle, pOverlappedEvent, &m_numBytesReturned, TRUE );
        }

        //-------------------------------------------------------------------------

        CloseHandle( pOverlappedEvent->hEvent );
        CloseHandle( m_pDirectoryHandle );
        m_directoryToWatch = Path();
        pOverlappedEvent->hEvent = nullptr;
        m_pDirectoryHandle = nullptr;
    }

    bool Watcher::Update()
    {
        EE_ASSERT( IsWatching() );
        m_unhandledEvents.clear();

        //-------------------------------------------------------------------------

        auto pOverlappedEvent = reinterpret_cast<OVERLAPPED*>( m_pOverlappedEvent );
        if ( GetOverlappedResult( m_pDirectoryHandle, pOverlappedEvent, &m_numBytesReturned, FALSE ) )
        {
            EE_ASSERT( m_numBytesReturned > 0 ); // 0 means we've overflowed the allocated buffer - we dont support that atm
            m_requestPending = false;

            ProcessListOfDirectoryChanges();
            RequestListOfDirectoryChanges();

            return true;
        }
        else // Error occurred or request not complete
        {
            if ( GetLastError() != ERROR_IO_INCOMPLETE ) // GetOverlappedResults returns 'ERROR_IO_INCOMPLETE' while the operation is pending
            {
                EE_LOG_FATAL_ERROR( "FileSystem", "FileSystemWatcher", "Failed to get overlapped results: %s", Platform::Win32::GetLastErrorMessage().c_str() );
                EE_HALT();
            }

            return false;
        }
    }

    //-------------------------------------------------------------------------

    namespace
    {
        static Path GetFileSystemPath( Path const& dirPath, _FILE_NOTIFY_EXTENDED_INFORMATION* pNotifyInformation )
        {
            EE_ASSERT( pNotifyInformation != nullptr );

            char strBuffer[256] = { 0 };
            wcstombs( strBuffer, pNotifyInformation->FileName, pNotifyInformation->FileNameLength / 2 );

            Path filePath = dirPath;
            filePath.Append( strBuffer );

            if ( pNotifyInformation->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                filePath.MakeIntoDirectoryPath();
            }
            return filePath;
        }
    }

    void Watcher::RequestListOfDirectoryChanges()
    {
        auto pOverlappedEvent = reinterpret_cast<OVERLAPPED*>( m_pOverlappedEvent );

        m_requestPending = ReadDirectoryChangesExW
        (
            m_pDirectoryHandle,
            m_resultBuffer,
            s_resultBufferSize,
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
            &m_numBytesReturned,
            pOverlappedEvent,
            NULL,
            ReadDirectoryNotifyExtendedInformation
        );

        EE_ASSERT( m_requestPending );
    }

    void Watcher::ProcessListOfDirectoryChanges()
    {
        if ( m_numBytesReturned != 0 )
        {
            _FILE_NOTIFY_EXTENDED_INFORMATION* pNotify = nullptr;
            size_t offset = 0;

            do
            {
                pNotify = reinterpret_cast<_FILE_NOTIFY_EXTENDED_INFORMATION*>( m_resultBuffer + offset );

                Event& newEvent = m_unhandledEvents.emplace_back();

                switch ( pNotify->Action )
                {
                    case FILE_ACTION_ADDED:
                    {
                        newEvent.m_path = GetFileSystemPath( m_directoryToWatch, pNotify );
                        newEvent.m_type = ( newEvent.m_path.IsDirectoryPath() ) ? Event::DirectoryCreated : Event::FileCreated;
                    }
                    break;

                    case FILE_ACTION_REMOVED:
                    {
                        newEvent.m_path = GetFileSystemPath( m_directoryToWatch, pNotify );
                        newEvent.m_type = ( newEvent.m_path.IsDirectoryPath() ) ? Event::DirectoryDeleted : Event::FileDeleted;
                    }
                    break;

                    case FILE_ACTION_MODIFIED:
                    {
                        newEvent.m_path = GetFileSystemPath( m_directoryToWatch, pNotify );
                        newEvent.m_type = ( newEvent.m_path.IsDirectoryPath() ) ? Event::DirectoryModified : Event::FileModified;
                    }
                    break;

                    case FILE_ACTION_RENAMED_OLD_NAME:
                    {
                        // Get old name and type
                        newEvent.m_oldPath = GetFileSystemPath( m_directoryToWatch, pNotify );
                        newEvent.m_type = ( newEvent.m_oldPath.IsDirectoryPath() ) ? Event::DirectoryRenamed : Event::FileRenamed;

                        // Get new name
                        EE_ASSERT( pNotify->NextEntryOffset != 0 );
                        offset += pNotify->NextEntryOffset;
                        pNotify = reinterpret_cast<_FILE_NOTIFY_EXTENDED_INFORMATION*>( m_resultBuffer + offset );
                        EE_ASSERT( pNotify->Action == FILE_ACTION_RENAMED_NEW_NAME );
                        newEvent.m_path = GetFileSystemPath( m_directoryToWatch, pNotify );
                    }
                    break;
                }

                offset += pNotify->NextEntryOffset;

            } while ( pNotify->NextEntryOffset != 0 );

            // Clear the result buffer
            Memory::MemsetZero( m_resultBuffer, s_resultBufferSize );
        }
    }
}
#endif