import com.skynet.codegenerator.*;
import com.skynet.codegenerator.meta.*;
import cn.hutool.json.JSONUtil;
import java.lang.reflect.InvocationTargetException;
import java.nio.file.*;
import java.util.*;

/** Observe authentic metadata and complete generation without replacing algorithms. */
public final class ConstantMetadataProbe {
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
            List<Map<String, Object>> metadata = new ArrayList<>();
            for (ObjectDescriptor entity : new EntityDomain(domain).getObjectDescriptors()) {
                Map<String, Object> entry = new LinkedHashMap<>();
                entry.put("entity", entity.getName());
                entry.put("constant", entity.isConstant());
                entry.put("root", entity.isRoot());
                List<Candidate> candidates = entity.getIdentityConstants();
                List<Map<String, Object>> values = new ArrayList<>();
                if (candidates != null) {
                    for (Candidate candidate : candidates) {
                        Map<String, Object> value = new LinkedHashMap<>();
                        value.put("id", candidate.getId());
                        value.put("key", candidate.getKey());
                        value.put("value", candidate.getValue());
                        value.put("line", candidate.getLineNumber());
                        values.add(value);
                    }
                }
                entry.put("candidates", candidates == null ? null : values);
                metadata.add(entry);
            }
            observation.put("metadata", metadata);
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
                    throw new IllegalArgumentException("Unsafe generated fixture path");
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
