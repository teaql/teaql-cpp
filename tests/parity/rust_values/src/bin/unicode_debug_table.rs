// Test oracle: records the standard-library Unicode Debug classification used
// by SqlCompileError::Display. Does not modify the TEAQL model generator.
fn main() {
    let mut ranges=Vec::new();let mut start=None;let mut last=0;
    for code in 0..=0x10ffff {
        let escaped=char::from_u32(code).is_some_and(|c|{let mut e=c.escape_debug();e.next()==Some('\\') && e.next()==Some('u')});
        if escaped {if start.is_none(){start=Some(code);}last=code;}
        else if let Some(first)=start.take(){ranges.push((first,last));}
    }
    if let Some(first)=start{ranges.push((first,last));}
    println!("// Rust Unicode {:?}; emitted by tests/parity/rust_values/src/bin/unicode_debug_table.rs",std::char::UNICODE_VERSION);
    println!("#pragma once\n#include <array>\n#include <utility>\nnamespace teaql::detail {{");
    println!("inline constexpr std::array<std::pair<char32_t,char32_t>,{}> debug_escape_ranges{{{{",ranges.len());
    for (first,last) in ranges {println!("    {{0x{first:X},0x{last:X}}},");}
    println!("}}}};\n}}");
}
