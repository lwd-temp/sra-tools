/*  $Id: fastq_parse.cpp 637208 2021-09-08 21:30:39Z shkeda $
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
* Author:  Many, by the time it's done.
*
* File Description:
*
* ===========================================================================
*/
// comand line 
#include "CLI11.hpp"
//loging 
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "fastq_error.hpp"
#include "fastq_parser.hpp"
#include "fastq_writer.hpp"

#include <json.hpp>

#if __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::experimental::filesystem;
#endif

using json = nlohmann::json;

#define LOCALDEBUG

//  ============================================================================
class CFastqParseApp
//  ============================================================================
{
public:    
    int AppMain(int argc, const char* argv[]);
private:
    int Run();
    int xRun();
    int xRunDigest();

    void xSetupInput();
    void xSetupOutput();
    
    // check consistency of digest data 
    // verify the same platform, the same platform
    void xProcessDigest(json& data);

    void xBreakdownOutput();
    void xCheckInputFiles(vector<string>& files);

    string mOutputFile;
    string mDestination; ///< path to sra archive
    bool mDebug{false};
    vector<char> mReadTypes;
    using TInptuFiles = vector<string>;
    vector<TInptuFiles> mInputBatches;
    bool mDiscardNames{false};
    bool mAllowEarlyFileEnd{false}; ///< Flag to continue if one of the streams ends
    int mQuality{-1};               ///< quality score interpretation (0, 33, 64)
    bool mDigest{false};            ///< Flag to produce summary of input
    string mSpotFile;
    string mNameColumn;             ///< NAME column's name
    ostream* mpOutStr{nullptr};
    shared_ptr<fastq_writer> m_writer;
};


void s_AddReadPairBatch(vector<string>& batch, vector<vector<string>>& out)
{
    if (batch.empty())
        return;
    if (out.empty()) {
        out.resize(batch.size());
    } else if (batch.size() != out.size()) {
        throw fastq_error(10, "Invalid command line parameters, inconsistent number of read pairs");
    }

    for (size_t i = 0; i < batch.size(); ++i) {
        out[i].push_back(move(batch[i]));
    }
}

static 
void s_split(const string& str, vector<string>& out, char c = ',')
{
    if (str.empty())
        return;
    auto begin = str.begin();
    auto end  = str.end();
    while (begin != end) {
        auto it = begin;
        while (it != end && *it != c) ++it;
        out.emplace_back(begin, it);
        if (it == end)
            break;
        begin = ++it;
    } 
}
//  ----------------------------------------------------------------------------
int CFastqParseApp::AppMain(int argc, const char* argv[])
{
    try {
        CLI::App app{"SharQ"};

        mOutputFile.clear();
        mDestination = "sra.out";

        app.add_option("--output", mDestination, "Output archive path");        

        string platform;
        app.add_option("--platform", platform, "Optional platform");

        string read_types;
        app.add_option("--readTypes", read_types, "file read types <B|T(s)>");

        app.add_option("--useAndDiscardNames", mDiscardNames, "Discard file names (Boolean)");

        app.add_flag("--allowEarlyFileEnd", mAllowEarlyFileEnd, "Complete load at early end of one of the files");

        app.add_flag("--debug", mDebug, "Debug mode");

        bool print_errors = false;
        app.add_flag("--help_errors,--help-errors", print_errors, "Print error codes and descriptions");

        bool no_timestamp = false;
        app.add_flag("--no-timestamp", no_timestamp, "No time stamp in debug mode");

        string log_level = "info";
        app.add_option("--log-level", log_level, "Log level")
            ->default_val("info")
            ->check(CLI::IsMember({"trace", "debug", "info", "warning", "error"}));

        string hash_file;
        app.add_option("--hash", hash_file, "Check hash file");
        app.add_option("--spot_file", mSpotFile, "Save spot names");

        app.add_option("--name-column", mNameColumn, "Database name for NAME column")
            ->default_str("NAME")
            ->default_val("NAME")
            ->check(CLI::IsMember({"NONE", "NAME", "RAW_NAME"}));

        vector<string> read_pairs(4);
        app.add_option("--read1PairFiles", read_pairs[0], "Read 1 files");
        app.add_option("--read2PairFiles", read_pairs[1], "Read 2 files");
        app.add_option("--read3PairFiles", read_pairs[2], "Read 3 files");
        app.add_option("--read4PairFiles", read_pairs[3], "Read 4 files");

        vector<string> input_files;
        app.add_option("files", input_files, "FastQ files to parse");

        mQuality = -1;
        app.add_option("--quality,-q", mQuality, "Interpretation of ascii quality")
            ->check(CLI::IsMember({0, 33, 64}));

        mDigest = false;
        app.add_flag("--digest", mDigest, "Summary of input files");

        CLI11_PARSE(app, argc, argv);
        if (print_errors) {
            fastq_error::print_error_codes(cout);
            return 0;
        }

        if (mDigest) 
            spdlog::set_level(spdlog::level::from_str("error"));
        else    
            spdlog::set_level(spdlog::level::from_str(log_level));

        if (!hash_file.empty()) {
            check_hash_file(hash_file);
            return 0;
        }

        xSetupOutput();
        if (mDigest == false) {
            if (mDebug) {
                m_writer = make_shared<fastq_writer>();
                if (no_timestamp)
                    spdlog::set_pattern("[%l] %v");

            } else {
                spdlog::set_pattern("%v");
                m_writer = make_shared<fastq_writer_vdb>(*mpOutStr);
            }
        }
        if (read_types.find_first_not_of("TBA") != string::npos)
            throw fastq_error(150, "Invalid --readTypes values '{}'", read_types);

        copy(read_types.begin(), read_types.end(), back_inserter(mReadTypes));

        if (!read_pairs[0].empty()) {
            if (mDigest == false && mReadTypes.empty())
                throw fastq_error(20, "No readTypes provided");
            for (auto p : read_pairs) {
                vector<string> b;
                s_split(p, b);
                xCheckInputFiles(b);
                s_AddReadPairBatch(b, mInputBatches);
            }
        } else {
            if (input_files.empty()) {
                mInputBatches.push_back({"-"});
            } else {
                stable_sort(input_files.begin(), input_files.end());
                xCheckInputFiles(input_files);
                fastq_reader::cluster_files(input_files, mInputBatches);
            }
        }
        return Run();
    } catch (fastq_error& e) {
        spdlog::error(e.Message());
    } catch(std::exception const& e) {
        spdlog::error("[code:0] Runtime error: {}", e.what());
    }
    return 1;
}


void CFastqParseApp::xCheckInputFiles(vector<string>& files)
{
    for (auto& f : files) {
        if (f == "-" || fs::exists(f)) continue;
        bool not_found = true;
        auto ext = fs::path(f).extension();
        if (ext != ".gz" && ext != ".bz2") {
            if (fs::exists(f + ".gz")) {
                spdlog::debug("File '{}': .gz extension added", f);
                f += ".gz";
                not_found = false;
            } else if (fs::exists(f + ".bz2")) {
                spdlog::debug("File '{}': .bz2 extension added", f);
                f += ".bz2";
                not_found = false;
            } 
        } else if (ext == ".gz" || ext == ".bz2") {
            auto new_fn = f.substr(0, f.size() - ext.string().size());
            if (fs::exists(new_fn)) {
                spdlog::debug("File '{}': {} extension ignored", f, ext.string());
                f = new_fn;
                not_found = false;
            }

        }
        if (not_found) 
            throw fastq_error(40, "File '{}' does not exist", f);
    }
}

//  ----------------------------------------------------------------------------
int CFastqParseApp::Run()
{
    int retStatus = mDigest ? xRunDigest() : xRun();
    xBreakdownOutput();
    return retStatus;
}


void CFastqParseApp::xProcessDigest(json& data)
{
    assert(data.contains("groups"));
    assert(data["groups"].front().contains("files"));
    const auto& first = data["groups"].front()["files"].front();
    int platform = first["platform_code"];
    bool is10x = first["is_10x"];
    int total_reads = 0;
    for (auto& gr : data["groups"]) {
        int max_reads = 0;
        int group_reads = 0;
        auto& files = gr["files"];
        for (auto& f : files) {
            if (mQuality != -1) 
                f["quality_encoding"] = mQuality; // Override quality
           if (platform != f["platform_code"]) 
                throw fastq_error(70, "Input files have deflines from different platforms {} != {}", platform, int(f["platform_code"]));
            if (is10x != f["is_10x"]) 
                throw fastq_error(80);// "Inconsistent submission: 10x submissions are mixed with different types.");
            max_reads = max<int>(max_reads, f["max_reads"]); 
            group_reads += (int)f["max_reads"];

            // if read types are specified (mix of B and T) 
            // and the reads in an interleaved file don't have read numbers 
            // and orphans are present
            // then sharq should fail. 
            if (!mReadTypes.empty() && max_reads > 1 && f["orphans"] && f["readNums"].empty()) 
                throw fastq_error(190); // "Usupported interleaved file with orphans"
        }

        // non10x, non interleaved files 
        // sort by readNumber
        if (is10x == false && max_reads == 1) {
            sort(files.begin(), files.end(), [](const auto& d1, const auto& d2){
                string v1 = d1["readNums"].empty() ? "" : d1["readNums"].front();
                string v2 = d2["readNums"].empty() ? "" : d2["readNums"].front();
                return v1 < v2;
            });
        }
        if (!mReadTypes.empty()) {
            if ((int)mReadTypes.size() != group_reads) 
                throw fastq_error(30, "readTypes number should match the number of reads ({})", group_reads);
        }
        total_reads = max<int>(group_reads, total_reads);
        gr["total_reads"] = total_reads;
    } 
    if (mReadTypes.empty()) {
        //auto num_files = data["groups"].front().size();
        if (is10x) 
            mReadTypes.resize(total_reads, 'A');
        else if (total_reads < 3) {
            mReadTypes.resize(total_reads, 'B');
        } else {
            throw fastq_error(20); // "No readTypes provided");
        }
    }    

    for (auto& gr : data["groups"]) {
        auto& files = gr["files"];
        int total_reads = gr["total_reads"];
        if ((int)mReadTypes.size() < total_reads)
            throw fastq_error(30, "readTypes number should match the number of reads");
        int j = 0;   
        // assign read types for each file
        for (auto& f : files) { 
            int num_reads = f["max_reads"];
            while (num_reads) {
                f["readType"].push_back(char(mReadTypes[j]));
                --num_reads;
                ++j;
            }
        }
    }

    switch ((int)first["quality_encoding"]) {
    case 0: 
        m_writer->set_attr("quality_expression", "(INSDC:quality:phred)QUALITY");
        break;
    case 33: 
        m_writer->set_attr("quality_expression", "(INSDC:quality:text:phred_33)QUALITY");
        break;
    case 64: 
        m_writer->set_attr("quality_expression", "(INSDC:quality:text:phred_64)QUALITY");
        break;
    default:
        throw runtime_error("Invaid quality encoding");
    }    
    m_writer->set_attr("platform", to_string(first["platform_code"]));

}


//  -----------------------------------------------------------------------------
int CFastqParseApp::xRunDigest()
{
    if (mInputBatches.empty())
        return 1;
    spdlog::set_level(spdlog::level::from_str("off"));
    json j;
    string error;
    try {
        get_digest(j, mInputBatches);
        //xProcessDigest(j);
    } catch (fastq_error& e) {
        error = e.Message();
    } catch(std::exception const& e) {
        error = fmt::format("[code:0] Runtime error: {}", e.what());
    }
    if (!error.empty()) {
        // remove special character if any
        error.erase(remove_if(error.begin(), error.end(),[](char c) { return !isprint(c); }), error.end());
        j["error"] = error;
    }
    
    cout << j.dump(4, ' ', true) << endl;
    return 0;
}

int CFastqParseApp::xRun()
{
    if (mInputBatches.empty())
        return 1;
    fastq_parser<fastq_writer> parser(m_writer);
    if (!mDebug)
        parser.set_spot_file(mSpotFile);
    parser.set_allow_early_end(mAllowEarlyFileEnd);
    json data;
    get_digest(data, mInputBatches);
    xProcessDigest(data);

    m_writer->set_attr("name_column", mNameColumn);
    m_writer->set_attr("destination", mDestination);
    m_writer->open();
    for (auto& group : data["groups"]) {
        parser.set_readers(group);
        parser.parse();
    }
    
    parser.check_duplicates();
    m_writer->close();
    return 0;
}


//  -----------------------------------------------------------------------------
void CFastqParseApp::xSetupOutput()
{
    mpOutStr = &cout;
    if (mOutputFile.empty()) 
        return;
    mpOutStr = dynamic_cast<ostream*>(new ofstream(mOutputFile, ios::binary));
    mpOutStr->exceptions(std::ofstream::badbit);
}



//  ----------------------------------------------------------------------------
void CFastqParseApp::xBreakdownOutput()
{
    if (mpOutStr != &cout) {
        delete mpOutStr;
    }
}


//  ----------------------------------------------------------------------------
int main(int argc, const char* argv[])
{
    ios_base::sync_with_stdio(false);   // turn off synchronization with standard C streams
    std::locale::global(std::locale("en_US.UTF-8")); // enable comma as thousand separator
    auto stderr_logger = spdlog::stderr_logger_mt("stderr"); // send log to stderr
    spdlog::set_default_logger(stderr_logger);
    
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v"); // default logging pattern
    return CFastqParseApp().AppMain(argc, argv);
}
