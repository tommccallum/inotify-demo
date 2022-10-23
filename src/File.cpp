#include <errno.h>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <iostream>
#include <cstring>
#include <map>
#include <vector>
#include <functional>
#include <algorithm>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <openssl/evp.h>
#include <errno.h>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <cctype>
#include <ftw.h>
#include <set>
#include "File.h"
#include "Base64.h"
#include "BoolOrError.h"

#define BUF_SIZE 1024

namespace
{
    thread_local std::vector<File> g_threaded_files;
    int threaded_crawl(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
    {
        g_threaded_files.emplace_back(fpath);
        return 0;
    }
}

File::File(std::string path) : m_path(path)
{
}

bool File::isSubpathOf(File const &other)
{
    if (abspath() == other.abspath())
    {
        return true;
    }
    if (other.isDirectory() == false)
    {
        return false;
    }
    std::string thisPath = dirname();
    std::string otherPath = other.dirname();
    // std::cout << "thisPath: " << thisPath << std::endl;
    // std::cout << "thisPath (full): " << abspath() << std::endl;
    // std::cout << "otherPath: " << otherPath << std::endl;
    // std::cout << "otherPath (full): " << other.abspath() << std::endl;
    if (thisPath.rfind(otherPath, 0) == 0)
    {
        return true;
    }
    return false;
}

std::string File::dirname() const
{
    if (exists() && isDirectory() == true)
    {
        return abspath();
    }
    else
    {
        if ( exists() ) {
            std::string filePath = abspath();
            auto p = filePath.rfind(separator());
            return filePath.substr(0, p);
        } else {
            std::string filePath = path();
            auto p = filePath.rfind(separator());
            return filePath.substr(0, p);
        }
    }
    return ""; // never actually gets here
}

std::string File::basename() const
{
    std::string filePath = abspath();
    auto p = filePath.rfind(separator());
    auto basename = filePath.substr(p + 1);
    return basename;
}

char File::separator() const
{
    return '/';
}

bool File::operator==(File const &other) const
{
    if (abspath() == other.abspath())
    {
        return true;
    }
    return false;
}

bool File::isDuplicate(File const & other) const {
    if (isDirectory() == false && other.isDirectory() == false)
    {
        auto a = digest();
        auto b = other.digest();
        return a == b;
    }
    return false;
}

std::string File::abspath() const
{
    std::string result;
    char buffer[PATH_MAX];
    memset(buffer, 0, PATH_MAX);
    realpath(m_path.c_str(), buffer);
    result.assign(buffer, strlen(buffer));
    return result;
}

std::vector<File> File::crawl()
{
    g_threaded_files.clear();
    m_files.clear();
    if (isDirectory() == false)
    {
        return m_files;
    }
    // recurse down, ignore symbolic links
    const int maxFileDescriptors = 1024;
    nftw(m_path.c_str(), threaded_crawl, maxFileDescriptors, FTW_DEPTH | FTW_PHYS);
    std::copy(g_threaded_files.begin(), g_threaded_files.end(), std::back_inserter(m_files));
    return m_files;
}

BoolOrError File::isDirectory() const
{
    struct stat s;
    auto result = stat(m_path.c_str(), &s);
    if (result != 0)
    {
        std::cerr << "Unable to stat " << m_path << std::endl;
        // TODO what to do here, throw exception?
        return BoolOrError::Error;
    }
    return CAST_BoolOrError(((s.st_mode & S_IFDIR) == S_IFDIR));
}

bool File::do_mkdir(std::string const & path, mode_t mode) const 
{
    struct stat st;
    bool status = true;
    if (stat(path.c_str(), &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path.c_str(), mode) != 0 && errno != EEXIST)
            status = false;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = false;
    }
    return(status);
}

bool File::mkpath(std::string const & path, mode_t mode)  const {
    size_t pp=0;
    size_t sp=0;
    bool status=true;
    std::string copypath = path.substr(0,1);

    while (status == true && (sp = path.find('/', pp)) != -1)
    {
        if (sp != pp)
        {
            copypath += path.substr(pp, sp-pp);
            /* Neither root nor double slash in path */
            status = do_mkdir(copypath, mode);
            copypath += '/';
        }
        pp = sp + 1;
    }
    if (status == true) {
        status = do_mkdir(path, mode);
    }
    return (status);
}

bool File::copyFrom(File const &source) const
{
    std::string sourceAbsPath = source.abspath();
    std::string destinationAbsPath = abspath();
    if (access(sourceAbsPath.c_str(), R_OK) != 0)
    {
        std::cout << "Unable to READ access " << sourceAbsPath << std::endl;
        return false;
    }
    if ( !exists() ) {
        destinationAbsPath = path();
        struct stat buffer;
        if ( source.isDirectory() == true ) {
            stat(source.abspath().c_str(), &buffer);
            if ( mkpath(destinationAbsPath.c_str(), buffer.st_mode ) ) {
                std::cout << "Created new directory: " << destinationAbsPath << std::endl;
            } else {
                std::cout << "Could not create directory " << destinationAbsPath << std::endl;
                return false;
            }
        } else {
            stat(source.dirname().c_str(), &buffer);
            std::string directoryToCreateIfNotExists = dirname();
            if ( mkpath(directoryToCreateIfNotExists, buffer.st_mode) ) {
                std::ifstream in(sourceAbsPath);
                std::ofstream out(destinationAbsPath);
                out << in.rdbuf();
                out.close();
                std::cout << "Created new file: " << destinationAbsPath << std::endl;
            } else {
                std::cerr << "Whilst creating new file, failed to make path: " << dirname() << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    } else {
        if (access(destinationAbsPath.c_str(), W_OK) != 0)
        {
            std::cout << "Unable to WRITE access " << destinationAbsPath << std::endl;
            return false;
        }
        std::ifstream in(sourceAbsPath);
        std::ofstream out(destinationAbsPath);
        out << in.rdbuf();
        std::cout << "Modified file: " << destinationAbsPath << std::endl;
    }
    return true;
}

bool File::exists () const {
  struct stat buffer;   
  return (stat (abspath().c_str(), &buffer) == 0); 
}

bool File::copyTo(File const &destination) const
{
    if (access(m_path.c_str(), R_OK) != 0)
    {
        std::cout << "Unable to READ access " << m_path << std::endl;
        return false;
    }
    std::string sourceAbsPath = abspath();
    std::string destinationAbsPath = destination.abspath();
    if ( !destination.exists() ) {
        struct stat buffer;
        if ( isDirectory() == true ) {
            stat(sourceAbsPath.c_str(), &buffer);
            if ( mkpath(destinationAbsPath.c_str(), buffer.st_mode)  ) {
                std::cout << "Created new directory: " << destinationAbsPath << std::endl;
            } else {
                std::cout << "Could not create directory " << destinationAbsPath << std::endl;
                return false;
            }
        } else {
            stat(dirname().c_str(), &buffer);
            if ( mkpath(dirname(), buffer.st_mode) ) {
                std::ifstream in(sourceAbsPath);
                std::ofstream out(destinationAbsPath);
                out << in.rdbuf();
                std::cout << "Created new file: " << destinationAbsPath << std::endl;
            } else {
                std::cerr << "Whilst creating new file, failed to make path: " << dirname() << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    } else {
        if (access(destinationAbsPath.c_str(), F_OK | W_OK) != 0)
        {
            std::cout << "Unable to receive WRITE access on " << destinationAbsPath << std::endl;
            return false;
        }
        std::ifstream in(sourceAbsPath);
        std::ofstream out(destinationAbsPath);
        out << in.rdbuf();
        std::cout << "Modified file: " << destinationAbsPath << std::endl;
    }
    return true;
}

std::string File::digest() const
{
    if (isDirectory() == true)
    {
        return "";
    }
    char buffer[BUF_SIZE];
    auto algo = EVP_sha3_384();
    EVP_MD_CTX *mdctx;

    if ((mdctx = EVP_MD_CTX_create()) == NULL)
    {
        std::cerr << "EVP_MD_CTX_create error" << std::endl;
        return "";
    }
    if (EVP_DigestInit_ex(mdctx, algo, NULL) != 1)
    {
        std::cerr << "Failed to initialize digest engine" << std::endl;
        return "";
    }

    int bytes_read = 0;
    auto fh = fopen(m_path.c_str(), "rb");
    while ((bytes_read = fread(buffer, BUF_SIZE, 1, fh)) > 0)
    {
        if (EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1)
        {
            std::cerr << "Error updating digest" << std::endl;
            fclose(fh);
            return "";
        }
    }
    fclose(fh);

    if (bytes_read < 0)
    {
        std::cerr << "Error reading file" << std::endl;
        return "";
    }

    unsigned int digest_len = EVP_MD_size(algo);
    unsigned char *digest;
    if ((digest = (unsigned char *)OPENSSL_malloc(digest_len)) == NULL)
    {
        std::cerr << "Error allocating memory for digest" << std::endl;
    }

    if (EVP_DigestFinal_ex(mdctx, digest, &digest_len) != 1)
    {
        OPENSSL_free(digest);
        std::cerr << "Failed to write digest" << std::endl;
        return "";
    }

    std::string result = base64(digest, digest_len);

    OPENSSL_free(digest);
    EVP_MD_CTX_destroy(mdctx);

    return result;
}

std::string File::path() const
{
    return m_path;
}

std::vector<File> const &File::files() const
{
    return m_files;
}

std::ostream &operator<<(std::ostream &out, File &file)
{
    std::string typeWord = "File";
    if (file.isDirectory() == true)
    {
        typeWord = "Directory";
    }
    out << typeWord << " " << file.path() << std::endl;
    if (file.isDirectory() == true)
    {
        if (file.m_files.size() > 0)
        {
            out << " - contains " << file.m_files.size() << " subitems";
        }
    }
    else
    {
        out << " - Digest " << file.digest();
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, File const &file)
{
    std::string typeWord = "File";
    if (file.isDirectory() == true)
    {
        typeWord = "Directory";
    }
    out << typeWord << " " << file.path() << std::endl;
    if (file.isDirectory() == true)
    {
        if (file.m_files.size() > 0)
        {
            out << " - contains " << file.m_files.size() << " subitems";
        }
    }
    else
    {
        out << " - Digest " << file.digest();
    }
    return out;
}
