// Standard-library Unicode casing observations for schema identifier naming.
// Independent of the TEAQL model generator; no runtime Unicode dependency.
fn main() {
    let mut singles=Vec::new();let mut expanded=Vec::new();
    for code in 0..=0x10ffff {
        if let Some(c)=char::from_u32(code) {
            let upper:Vec<u32>=c.to_uppercase().map(u32::from).collect();
            if upper.len()==1 {if upper[0]!=code {singles.push((code,upper[0] as i32-code as i32));}}
            else {assert!(upper.len()<=3);expanded.push((code,upper));}
        }
    }
    let mut ranges=Vec::new();let mut i=0;
    while i<singles.len() {
        let (first,delta)=singles[i];let mut last=first;let mut step=1;
        if i+1<singles.len() && singles[i+1].1==delta && singles[i+1].0-first<=2 {
            step=singles[i+1].0-first;
            while i+1<singles.len() && singles[i+1].1==delta && singles[i+1].0-last==step {i+=1;last=singles[i].0;}
        }
        ranges.push((first,last,step,delta));i+=1;
    }
    println!("// Rust Unicode {:?}; emitted by tests/parity/rust-values/src/bin/unicode_uppercase_table.rs",std::char::UNICODE_VERSION);
    println!("#pragma once\n#include <array>\n#include <cstdint>\nnamespace teaql::detail {{");
    println!("struct UppercaseRange {{char32_t first,last; std::uint32_t step; std::int32_t delta;}};");
    println!("inline constexpr std::array<UppercaseRange,{}> uppercase_ranges{{{{",ranges.len());
    for (first,last,step,delta) in ranges {println!("    {{0x{first:X},0x{last:X},{step},{delta}}},");}
    println!("}}}};\nstruct UppercaseExpansion {{char32_t from; std::array<char32_t,3> to;}};");
    println!("inline constexpr std::array<UppercaseExpansion,{}> uppercase_expansions{{{{",expanded.len());
    for (code,mut to) in expanded {to.resize(3,0);println!("    {{0x{code:X},{{0x{:X},0x{:X},0x{:X}}}}},",to[0],to[1],to[2]);}
    println!("}}}};\n}}");
}
