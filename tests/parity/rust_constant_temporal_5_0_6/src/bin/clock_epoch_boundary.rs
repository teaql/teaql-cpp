fn main() {
    match std::env::args().nth(1).unwrap().as_str() {
        "timestamp" => println!("{}", teaql_core::time::Timestamp::now().as_millis()),
        "utc" => println!("{}", chrono::Utc::now().timestamp_millis()),
        "negative_value" => println!("{},{}", teaql_core::time::Timestamp(-1).as_millis(),
                                    chrono::DateTime::<chrono::Utc>::from_timestamp_millis(-1).unwrap().timestamp_millis()),
        _ => panic!("unknown clock boundary mode"),
    }
}
