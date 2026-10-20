package com.skynet.codegenerator.cpp;

import com.fasterxml.jackson.databind.ObjectMapper;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/** Observe descriptor metadata effects; typed JSON execution has a separate compiled gate. */
public final class ContainerMetadataProbe {
  public static void main(String[] args) throws Exception {
    ObjectMapper mapper = new ObjectMapper();
    BufferedReader input = new BufferedReader(new InputStreamReader(System.in));
    for (String line; (line = input.readLine()) != null; ) {
      String value = mapper.readValue(line, String.class);
      Map<String, Object> output = new LinkedHashMap<>();
      try {
        var metadata = CppRustLiteralMetadata.auditMetadata(value);
        output.put("parsed", true);
        output.put("entity", metadata.entity() == null ? "Probe" : metadata.entity());
        output.put("table", metadata.table() == null ? "probe_data" : metadata.table());
        output.put("service", metadata.dataService());
        output.put("indexed", metadata.indexedLayout());
        output.put("limit", metadata.limit());
        output.put("masks", metadata.masks() == null ? List.of() : metadata.masks());
        output.put("declared", metadata.masks() != null);
        output.put("relations", metadata.reverseRelations().stream().map(r -> Map.of(
            "name", r.name(), "target", r.target(), "local", r.localKey(),
            "foreign", r.foreignKey(), "many", r.many(), "typed", r.jsonType() != null)).toList());
      } catch (IllegalArgumentException error) {
        output.put("parsed", false);
        output.put("stage", "generation");
        output.put("error", error.getMessage());
      }
      System.out.println(mapper.writeValueAsString(output));
    }
  }
}
