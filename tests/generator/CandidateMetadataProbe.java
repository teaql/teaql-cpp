import com.skynet.codegenerator.*;
import com.skynet.codegenerator.meta.*;
import com.skynet.codegenerator.naming.RustFieldDescriptorNaming;
import cn.hutool.json.JSONUtil;
import java.lang.reflect.InvocationTargetException;
import java.nio.file.*;
import java.util.*;

/** Observe complete scalar candidate metadata and authentic generated outputs. */
public final class CandidateMetadataProbe {
    public static void main(String[] args) throws Exception {
        String mode = args[0];
        Path input = Path.of(args[1]), output = Path.of(args[2]);
        Files.createDirectories(output);
        Map<String, Object> observation = new LinkedHashMap<>();
        try {
            GenerateContext context = new GenerateContext(
                mode.equals("rust") ? "rust-lib-core" : "cpp-lib-core", input.getFileName().toString());
            context.setEntranceFile(input.getFileName().toString());
            context.appendFile(input.getFileName().toString(), Files.readAllBytes(input));
            BaseDomainManager manager = new BaseDomainManager();
            Domain domain = manager.getDomain(manager.parse(context));
            List<Map<String, Object>> fields = new ArrayList<>();
            for (ObjectDescriptor entity : new EntityDomain(domain).getObjectDescriptors()) {
                for (FieldDescriptor field : entity.getFields()) {
                    List<Candidate> candidates = RustFieldDescriptorNaming.rustCandidates(field);
                    if (candidates.isEmpty()) {
                        continue;
                    }
                    Map<String, Object> entry = new LinkedHashMap<>();
                    entry.put("entity", entity.getName());
                    entry.put("field", field.getName());
                    entry.put("type", RustFieldDescriptorNaming.rustType(field));
                    List<Map<String, Object>> values = new ArrayList<>();
                    for (Candidate candidate : candidates) {
                        Map<String, Object> value = new LinkedHashMap<>();
                        value.put("id", candidate.getId());
                        value.put("key", candidate.getKey());
                        value.put("value", candidate.getValue());
                        value.put("method", candidate.getRustCandidateMethodValue());
                        value.put("filter", candidate.getRustCandidateFilterValue());
                        values.add(value);
                    }
                    entry.put("candidates", values);
                    fields.add(entry);
                }
            }
            observation.put("fields", fields);
            List<GeneratedFile> files;
            if (mode.equals("rust")) {
                files = new DomainGenerationService().generateDomainFiles(domain, "rust-lib-core");
            } else {
                Class<?> generator = Class.forName("com.skynet.codegenerator.cpp.CppCoreGenerator");
                Object instance = generator.getConstructor(boolean.class, boolean.class).newInstance(false, true);
                @SuppressWarnings("unchecked")
                List<GeneratedFile> generated = (List<GeneratedFile>) generator.getMethod("generate", Domain.class)
                    .invoke(instance, domain);
                files = generated;
            }
            for (GeneratedFile file : files) {
                Path target = output.resolve(file.getPath()).normalize();
                if (!target.startsWith(output)) {
                    throw new IllegalArgumentException("Unsafe generated candidate path");
                }
                Files.createDirectories(target.getParent());
                Files.write(target, file.getData());
            }
            observation.put("generated", true);
            observation.put("files", files.size());
        } catch (Exception exception) {
            Throwable error = exception instanceof InvocationTargetException ? exception.getCause() : exception;
            observation.put("generated", false);
            observation.put("error_type", error.getClass().getName());
            observation.put("error", error.getMessage());
        }
        Files.writeString(output.resolve("OBSERVATION.json"), JSONUtil.toJsonStr(observation) + "\n");
    }
}
