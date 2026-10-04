#include <teaql/provider_sqlite/detail/schema.hpp>
#include <algorithm>
namespace teaql::detail {
namespace {
struct UnicodeRange{char32_t first,last;};
struct LowerMapping{char32_t source,target[3];unsigned count;};
#include "unicode_lower_data.inc"
char32_t decode(std::string_view text,std::size_t& i){
    auto invalid=[]()->char32_t{throw std::invalid_argument("Invalid UTF-8 in SQLite identifier");};
    auto first=static_cast<unsigned char>(text[i++]);if(first<128)return first;
    unsigned count=first>=0xc2 && first<=0xdf?1:first>=0xe0 && first<=0xef?2:first>=0xf0 && first<=0xf4?3:0;
    if(!count || i+count>text.size())return invalid();
    char32_t value=first&((1u<<(6-count))-1);
    for(unsigned k=0;k<count;++k){auto byte=static_cast<unsigned char>(text[i++]);if((byte&0xc0)!=0x80)return invalid();value=(value<<6)|(byte&63);}
    if(value<(count==1?0x80u:count==2?0x800u:0x10000u) || value>0x10ffff || (value>=0xd800 && value<=0xdfff))return invalid();
    return value;
}
void append(String& out,char32_t c){if(c<128)out+=char(c);else if(c<0x800){out+=char(0xc0|(c>>6));out+=char(0x80|(c&63));}else if(c<0x10000){out+=char(0xe0|(c>>12));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}else{out+=char(0xf0|(c>>18));out+=char(0x80|((c>>12)&63));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}}
template<std::size_t N> bool member(char32_t c,const UnicodeRange(&ranges)[N]){auto at=std::upper_bound(std::begin(ranges),std::end(ranges),c,[](char32_t c,const UnicodeRange& range){return c<range.first;});return at!=std::begin(ranges) && c<=(at-1)->last;}
bool cased_after(std::string_view text,std::size_t i){while(i<text.size()){auto c=decode(text,i);if(!member(c,case_ignorable))return member(c,nonignored_cased);}return false;}
bool cased_before(std::string_view text,std::size_t i){while(i){--i;while(i && (static_cast<unsigned char>(text[i])&0xc0)==0x80)--i;auto next=i;auto c=decode(text,next);if(!member(c,case_ignorable))return member(c,nonignored_cased);}return false;}
}
bool sqlite_schema_utf8_valid(std::string_view text) noexcept{try{for(std::size_t i=0;i<text.size();)decode(text,i);return true;}catch(...){return false;}}
String sqlite_identifier_lower(Memory r,std::string_view text){
    String out(r);out.reserve(text.size());for(std::size_t i=0;i<text.size();){auto start=i;auto c=decode(text,i);
        if(c==0x3a3){append(out,cased_before(text,start) && !cased_after(text,i)?0x3c2:0x3c3);continue;}
        auto found=std::lower_bound(std::begin(lower_mappings),std::end(lower_mappings),c,[](const LowerMapping& item,char32_t c){return item.source<c;});
        if(found!=std::end(lower_mappings) && found->source==c){for(unsigned n=0;n<found->count;++n)append(out,found->target[n]);}else append(out,c);
    }return out;
}
}
