#ifndef FILE_H
#define FILE_H

#include <string>
#include <iostream>
#include <vector>
#include "BoolOrError.h"

class File
{
public:
    File(std::string path);
    std::string path() const;
    char separator() const;
    bool isSubpathOf(File const &other);
    std::string dirname() const;
    std::string basename() const;

    bool operator==(File const &other) const;
    bool isDuplicate(File const & other) const ;

    std::string abspath() const;
    bool exists () const;

    bool do_mkdir(std::string const & path, mode_t mode) const;
    bool mkpath(std::string const &  path, mode_t mode) const;

    std::vector<File> crawl();

    BoolOrError isDirectory() const;
    bool copyFrom(File const &source) const;
    bool copyTo(File const &destination) const;
    std::string digest() const;

    std::vector<File> const &files() const;

    friend std::ostream &operator<<(std::ostream &out, File &file);
    friend std::ostream &operator<<(std::ostream &out, File const &file);

private:
    std::string m_path;
    std::vector<File> m_files;
};

std::ostream &operator<<(std::ostream &out, File &file);
std::ostream &operator<<(std::ostream &out, File const &file);

#endif /* FILE_H */
