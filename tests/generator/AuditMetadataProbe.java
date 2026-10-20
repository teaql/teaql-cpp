package com.skynet.codegenerator.cpp;

import com.fasterxml.jackson.databind.ObjectMapper;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/** Observe the installed generator's effective audit interpretation, not a copied parser. */
public final class AuditMetadataProbe {
  public static void main(String[] args) throws Exception {
    ObjectMapper mapper = new ObjectMapper();
    BufferedReader input = new BufferedReader(new InputStreamReader(System.in));
    for (String line; (line = input.readLine()) != null; ) {
      String value = mapper.readValue(line, String.class);
      Map<String, Object> output = new LinkedHashMap<>();
      try {
        var metadata = CppRustLiteralMetadata.auditMetadata(value);
        output.put("parsed", true);
        output.put("limit", metadata.limit());
        output.put("masks", metadata.masks() == null ? List.of() : metadata.masks());
        output.put("declared", metadata.masks() != null);
      } catch (IllegalArgumentException error) {
        output.put("parsed", false);
        output.put("stage", "generation");
        output.put("error", error.getMessage());
      }
      System.out.println(mapper.writeValueAsString(output));
    }
  }
}
