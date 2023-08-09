/*==============================================================================
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
*/

#include "formatter.hpp"

#include <algorithm>

using namespace std;

Formatter::Format
Formatter::StringToFormat( const string & value )
{
    string lowercase = value;
    std::transform(
        lowercase.begin(),
        lowercase.end(),
        lowercase.begin(),
        [](unsigned char c){ return tolower(c); }
    );
    if ( lowercase == "csv" ) return CSV;
    if ( lowercase == "xml" ) return XML;
    if ( lowercase == "json" ) return Json;
    if ( lowercase == "tab" ) return Tab;
    throw VDB::Error( string("Invalid value for the --format option: ") + value );
}

Formatter::Formatter( Format f, uint32_t l )
: fmt( f ), limit( l ), first ( true ), count( 0 )
{
}

Formatter::~Formatter()
{
}

string
Formatter::formatJsonSeparator( void ) const
{
    if ( first ) {
        Formatter * ncThis = const_cast<Formatter *>(this);
        ncThis->first = false;
        return "";
    }
    else
        return ",";
}

void
Formatter::expectSingleQuery( const string & error ) const
{
    Formatter * ncThis = const_cast<Formatter *>(this);
    if ( ++ncThis->count > 1 )
        throw VDB::Error( error );
}

string
JoinPlatforms( const SraInfo::Platforms & platforms,
               const string & separator,
               const string & prefix = string(),
               const string & suffix = string()
)
{
    string ret;
    bool first = true;
    for( auto p : platforms )
    {
        if ( ! first )
        {
            ret += separator;
        }
        ret+= prefix + p + suffix;
        first = false;
    }
    return ret;
}

string
Formatter::format( const SraInfo::Platforms & platforms ) const
{
    switch ( fmt )
    {
    case Default:
        // default format, 1 value per line
        return JoinPlatforms( platforms, "\n", "PLATFORM: " );
    case CSV:
        // CSV, all values on 1 line
        expectSingleQuery( "CVS format does not support multiple queries" );
        return JoinPlatforms( platforms, "," );
    case XML:
        // XML, each value in a tag, one per line
        return " <PLATFORMS>\n" + 
            JoinPlatforms( platforms, "\n", "  <platform>", "</platform>" )
            + "\n </PLATFORMS>";
    case Json:
    {
        // Json, array of strings
        string out;
        const string separator(formatJsonSeparator());
        if (!separator.empty())
            out = " " + separator + "\n";
        out += string(" \"PLATFORMS\": [\n")
            + JoinPlatforms( platforms, ",\n", "  \"", "\"" ) + "\n ]";
        return out;
    }
    case Tab:
        // Tabbed, all values on 1 line
        expectSingleQuery("TAB format does not support multiple queries");
        return JoinPlatforms( platforms, "\t" );
    default:
        throw VDB::Error( "unsupported formatting option");
    }
}

string
Formatter::start( void ) const
{
    switch ( fmt )
    {
    case Default:
    case CSV:
    case Tab:
        return "";
    case XML:
        return "<SRA_INFO>";
    case Json:
        return "{";
    default:
        throw VDB::Error( "unsupported formatting option");
    }
}

string
Formatter::end( void ) const
{
    switch ( fmt )
    {
    case Default:
    case CSV:
    case Tab:
        return "";
    case XML:
        return "</SRA_INFO>";
    case Json:
        return "}";
    default:
        throw VDB::Error( "unsupported formatting option");
    }
}

string
Formatter::format( const string & value, const string & name ) const
{
    const string space(" ");

    switch ( fmt )
    {
    case CSV:
        expectSingleQuery("CVS format does not support multiple queries");
        return value;
    case Tab:
        expectSingleQuery( "TAB format does not support multiple queries" );
        return value;
    case Default:
        return name + ": " + value;
    case XML:
        return space + "<" + name + ">"
            + value + "</" + name + ">";
    case Json:
    {
        string out;
        const string separator(formatJsonSeparator());
        if (!separator.empty())
            out = space + separator + "\n";
        out += space + "\"" + name + string("\": \"") + value + "\"";
        return out;
    }
    default:
        throw VDB::Error( "unsupported formatting option");
    }
}

class SimpleSchemaDataFormatter : public VDB::SchemaDataFormatter {
    const std::string _space;
    int _indent;
public:
    SimpleSchemaDataFormatter(const std::string &space, int indent) :
        _space(space), _indent(indent)
    {}
    void format(
        const struct VDB::SchemaData &d, int indent = -1, bool first = true)
    {
        if (indent < 0)
            indent = _indent;
        for (int i = 0; i < indent; ++i)
            out += _space;
        out += d.name + "\n";
        for (auto it = d.parent.begin(); it < d.parent.end(); ++it)
            format(*it, indent + 1);
    }
};

class FullSchemaDataFormatter : public VDB::SchemaDataFormatter {
    const std::string _space;
    int _indent;
    const std::string _open;
    const std::string _openNext;
    const std::string _closeName;
    const std::string _close;
    const std::string _openParent1;
    const std::string _openParent2;
    const std::string _closeParent1;
    const std::string _closeParent2;
    const std::string _noParent;
public:
    FullSchemaDataFormatter(int indent, const std::string &space,
        const std::string &open, const std::string &openNext,
        const std::string &closeName, const std::string &close,
        const std::string &openParent1 = "",
        const std::string &openParent2 = "",
        const std::string &closeParent1 = "",
        const std::string &closeParent2 = "",
        const std::string &noParent = "")
        :
        _space(space), _indent(indent), _open(open), _openNext(openNext),
        _closeName(closeName), _close(close), _openParent1(openParent1),
        _openParent2(openParent2), _closeParent1(closeParent1),
        _closeParent2(closeParent2), _noParent(noParent)
    {}
    void format(
        const struct VDB::SchemaData &d, int indent = -1, bool first = true)
    {
        if (indent < 0)
            indent = _indent;
        if (!first) out += _openNext;

        for (int i = 0; i < indent; ++i) out += _space;
        out += _open + d.name + _closeName;

        if (!d.parent.empty()) {
            out += _openParent1;
            if (!_openParent2.empty()) {
                for (int i = 0; i < indent; ++i) out += _space;
                out += _openParent2;
                ++indent;
            }

            bool frst = true;
            for (auto it = d.parent.begin(); it < d.parent.end(); it++) {
                format(*it, indent + 1, frst);
                frst = false;
            }

            if (!_closeParent2.empty()) {
                out += _closeParent1;
                for (int i = 0; i < indent; ++i) out += _space;
                out += _closeParent2;
            }
            if (!_openParent2.empty())
                --indent;
        }
        else
            out += _noParent;

        for (int i = 0; i < indent; ++i) out += _space;
        out += _close;
    }
};

string
Formatter::format( const SraInfo::SpotLayouts & layouts, SraInfo::Detail detail ) const
{
    ostringstream ret;

    size_t count = layouts.size();
    if ( limit != 0 && limit < count )
    {
        count = limit;
    }

    switch ( fmt )
    {
    case Default:
        {
            bool first_group = true;
            for( size_t i = 0; i < count; ++i )
            {
                if ( first_group )
                {
                    first_group = false;
                }
                else
                {
                    ret << endl;
                }

                const SraInfo::SpotLayout & l = layouts[i];
                bool  first = true;
                ret << "SPOT: " << l.count
                    << ( l.count == 1 ? " spot: " : " spots: " );
                switch( detail )
                {
                case SraInfo::Short: ret << l.reads.size() << " reads"; break;
                case SraInfo::Abbreviated:
                    for ( auto r : l.reads )
                    {
                        ret << r.Encode( detail );
                    }
                    break;
                default:
                    for ( auto r : l.reads )
                    {
                        if ( first )
                        {
                            first = false;
                        }
                        else
                        {
                            ret << ", ";
                        }
                        ret << r.Encode( detail );
                    }
                }
            }
        }
        break;

    case Json:
        {
            const string separator(formatJsonSeparator());
            if (!separator.empty())
                ret << " " << separator << endl;
            ret << " \"SPOTS\": [" << endl;
            bool  first_layout = true;
            for( size_t i = 0; i < count; ++i )
            {
                const SraInfo::SpotLayout & l = layouts[i];
                if ( first_layout )
                {
                    first_layout = false;
                }
                else
                {
                    ret << "," << endl;
                }

                bool  first_read = true;
                ret << "  { \"count\": " << l.count << ", \"reads\": ";

                switch( detail )
                {
                case SraInfo::Short: ret << l.reads.size(); break;
                case SraInfo::Abbreviated:
                    ret << "\"";
                    for ( auto r : l.reads )
                    {
                        ret << r.Encode( detail );
                    }
                    ret << "\"";
                    break;
                default:
                    ret << "[";
                    for ( auto r : l.reads )
                    {
                        if ( first_read )
                        {
                            first_read = false;
                        }
                        else
                        {
                            ret << ", ";
                        }
                        ret << "{ \"type\": \"" << r.TypeAsString(detail) << "\", \"length\": " << r.length << " }";
                    }
                    ret << "]";
                }

                ret << " }";
            }
            ret << endl << " ]" << endl;
        }
        break;

    case CSV:
        expectSingleQuery( "CVS format does not support multiple queries" );
        for( size_t i = 0; i < count; ++i )
        {
            const SraInfo::SpotLayout & l = layouts[i];
            switch( detail )
            {
            case SraInfo::Short:
                ret << l.count << ", " << l.reads.size() << ", ";
                break;
            case SraInfo::Abbreviated:
                ret << l.count << ", ";
                for ( auto r : l.reads )
                {
                    ret << r.Encode(detail);
                }
                break;
            default:
                ret << l.count << ", ";
                for ( auto r : l.reads )
                {
                    ret << r.TypeAsString(detail) << ", " << r.length << ", ";
                }
            }
            ret << endl;
        }
        break;

    case Tab:
        expectSingleQuery("TAB format does not support multiple queries");
        for( size_t i = 0; i < count; ++i )
        {
            const SraInfo::SpotLayout & l = layouts[i];
            switch( detail )
            {
            case SraInfo::Short:
                ret << l.count << "\t" << l.reads.size() << "\t";
                break;
            case SraInfo::Abbreviated:
                ret << l.count << "\t";
                for ( auto r : l.reads )
                {
                    ret << r.Encode(detail);
                }
                break;
            default:
                ret << l.count << "\t";
                for ( auto r : l.reads )
                {
                    ret << r.TypeAsString(detail)  << "\t" << r.length << "\t";
                }
            }
            ret << endl;
        }
        break;

    case XML:
        ret << " <SPOTS>" << endl;
        for( size_t i = 0; i < count; ++i )
        {
            const SraInfo::SpotLayout & l = layouts[i];
            ret << "  <layout><count>" << l.count << "</count>";
            switch( detail )
            {
            case SraInfo::Short:
                ret << "<reads>" << l.reads.size() << "</reads>";
                break;
            case SraInfo::Abbreviated:
                ret << "<reads>";
                for ( auto r : l.reads )
                {
                    ret << r.Encode(detail);
                }
                ret << "</reads>";
                break;
            default:
                for ( auto r : l.reads )
                {
                    ret << "<read><type>" << r.TypeAsString(detail) << "</type><length>" << r.length << "</length></read>";
                }
            }

            ret << "</layout>" << endl;
        }
        ret << " </SPOTS>";
        break;

    default:
        throw VDB::Error( "unsupported formatting option");
    }

    return ret.str();
}

string Formatter::format(const VDB::SchemaInfo & info) const
{
    string space(" ");
    int indent(3);
    bool first(true);
    string out;

    switch ( fmt )
    {
    case Default:
    {
        indent = 2;
        out = "SCHEMA:\n"

            + space + "DBS:\n";
        SimpleSchemaDataFormatter db(" ", indent);
        for (auto it = info.db.begin(); it < info.db.end(); it++)
            db.format(*it);
        out += db.out;

        out += space + "TABLES:\n";
        SimpleSchemaDataFormatter table(" ", indent);
        for (auto it = info.table.begin(); it < info.table.end(); it++)
            table.format(*it);
        out += table.out;

        out += space + "VIEWS:\n";
        SimpleSchemaDataFormatter view(" ", indent);
        for (auto it = info.view.begin(); it < info.view.end(); it++)
            view.format(*it);
        out += view.out;

        break;
    }

    case Json:
    {
        const string separator(formatJsonSeparator());
        if (!separator.empty())
            out = space + separator + "\n";
        out += space + "\"SCHEMA\": {\n"

            + space + space + "\"DBS\": [\n";
        FullSchemaDataFormatter db(indent, " ",
            "{ \"Db\": \"", ",\n", "\"", "}",
            ",\n", " \"Parents\": [\n", "\n", "]\n", "\n");
        first = true;
        for (auto it = info.db.begin(); it < info.db.end(); it++) {
            db.format(*it, indent, first);
            first = false;
        }
        out += db.out;
        out += space + space + "],\n"

            + space + space + "\"TABLES\": [\n";
        FullSchemaDataFormatter table(indent, " ",
            "{ \"Tbl\": \"", ",\n", "\"", "}",
            ",\n", " \"Parents\": [\n", "\n", "]\n", "\n");
        first = true;
        for (auto it = info.table.begin(); it < info.table.end(); it++) {
            table.format(*it, indent, first);
            first = false;
        }
        out += table.out;
        out += "\n"
            + space + space + "],\n"

            + space + space + "\"VIEWS\": [\n";
        first = true;
        FullSchemaDataFormatter view(indent, " ",
            "{ \"View\": \"", ",\n", "\"", "}",
            ",\n", " \"Parents\": [\n", "\n", "]\n", "\n");
        for (auto it = info.view.begin(); it < info.view.end(); it++) {
            view.format(*it, indent, first);
            first = false;
        }
        out += view.out;
        out += space + space + "]\n"

            + space + "}";
        break;
    }

    case XML:
    {
        out = space + "<SCHEMA>\n"

            + space + space + "<DBS>\n";
        FullSchemaDataFormatter db(indent, space,
            "<Db>", "", "\n", "</Db>\n");
        for (auto it = info.db.begin(); it < info.db.end(); it++)
            db.format(*it);
        out += db.out;
        out += space + space + "</DBS>\n"

            + space + space + "<TABLES>\n";
        FullSchemaDataFormatter table(indent, space,
            "<Tbl>", "", "\n", "</Tbl>\n");
        for (auto it = info.table.begin(); it < info.table.end(); it++)
            table.format(*it);
        out += table.out;
        out += space + space + "</TABLES>\n"

            + space + space + "<VIEWS>\n";
        FullSchemaDataFormatter view(indent, space,
            "<View>", "", "\n", "</View>\n");
        for (auto it = info.view.begin(); it < info.view.end(); it++)
            view.format(*it);
        out += view.out;
        out += space + space + "</VIEWS>\n"

            + space + "</SCHEMA>";

        break;
    }

    default:
        throw VDB::Error( "unsupported formatting option for schema" );
    }

    return out;
}
