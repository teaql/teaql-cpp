import com.skynet.codegenerator.*;
import com.skynet.codegenerator.meta.*;
import com.skynet.codegenerator.naming.RustFieldDescriptorNaming;
import cn.hutool.json.JSONUtil;
import java.lang.reflect.InvocationTargetException;
import java.nio.file.*;
import java.util.*;

/** Observe real field/type configuration and complete frozen producer output. */
public final class TypeConfigurationProbe {
    public static void main(String[] args) throws Exception {
        String mode = args[0];
        Path input = Path.of(args[1]), output = Path.of(args[2]);
        Files.createDirectories(output);
        Map<String, Object> result = new LinkedHashMap<>();
        try {
            GenerateContext context = new GenerateContext(mode.equals("rust") ? "rust-lib-core" : "cpp-lib-core", input.getFileName().toString());
            context.setEntranceFile(input.getFileName().toString());
            context.appendFile(input.getFileName().toString(), Files.readAllBytes(input));
            BaseDomainManager manager = new BaseDomainManager();
            Domain domain = manager.getDomain(manager.parse(context));
            List<Map<String, Object>> fields = new ArrayList<>();
            for (ObjectDescriptor entity : new EntityDomain(domain).getObjectDescriptors()) {
                for (FieldDescriptor field : entity.getFields()) {
                    Map<String, Object> item = new LinkedHashMap<>();
                    item.put("entity", entity.getName());
                    item.put("field", field.getName());
                    item.put("type_name", field.getFieldType().getName());
                    item.put("field_rust_type", field.getAttributeValue("rustType"));
                    item.put("type_rust_type", field.getFieldType().getAttributeValue("rustType"));
                    item.put("storage_type", RustFieldDescriptorNaming.rustStorageType(field));
                    item.put("root_seed", RustFieldDescriptorNaming.rustRootSeedValue(field));
                    if (entity.isConstant() && entity.getIdentityConstants() != null && !entity.getIdentityConstants().isEmpty()) {
                        item.put("constant_seed", RustFieldDescriptorNaming.rustConstantSeedValue(field, entity.getIdentityConstants().get(0)));
                    }
                    fields.add(item);
                }
            }
            result.put("fields", fields);
            List<GeneratedFile> files;
            if (mode.equals("rust")) {
                files = new DomainGenerationService().generateDomainFiles(domain, "rust-lib-core");
            } else {
                Class<?> generator = Class.forName("com.skynet.codegenerator.cpp.CppCoreGenerator");
                Object instance = generator.getConstructor(boolean.class, boolean.class).newInstance(false, true);
                @SuppressWarnings("unchecked")
                List<GeneratedFile> generated = (List<GeneratedFile>) generator.getMethod("generate", Domain.class).invoke(instance, domain);
                files = generated;
            }
            for (GeneratedFile file : files) {
                Path target = output.resolve(file.getPath()).normalize();
                if (!target.startsWith(output)) {
                    throw new IllegalArgumentException("Unsafe type-configuration output path");
                }
                Files.createDirectories(target.getParent());
                Files.write(target, file.getData());
            }
            result.put("generated", true);
            result.put("file_count", files.size());
        } catch (Exception exception) {
            Throwable error = exception instanceof InvocationTargetException ? exception.getCause() : exception;
            result.put("generated", false);
            result.put("error_type", error.getClass().getName());
            result.put("error", error.getMessage());
        }
        Files.writeString(output.resolve("OBSERVATION.json"), JSONUtil.toJsonStr(result) + "\n");
    }
}
