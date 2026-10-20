import com.skynet.codegenerator.*;
import com.skynet.codegenerator.meta.*;
import com.skynet.codegenerator.naming.RustFieldDescriptorNaming;
import cn.hutool.json.JSONUtil;
import java.lang.reflect.Method;
import java.nio.file.*;
import java.util.*;

/** Observe original literal aliases separately from native identifier adaptation. */
public final class QueryAliasMetadataProbe {
    public static void main(String[] args) throws Exception {
        Path input = Path.of(args[0]);
        GenerateContext context = new GenerateContext("cpp-lib-core", input.getFileName().toString());
        context.setEntranceFile(input.getFileName().toString());
        context.appendFile(input.getFileName().toString(), Files.readAllBytes(input));
        BaseDomainManager manager = new BaseDomainManager();
        Domain domain = manager.getDomain(manager.parse(context));
        Class<?> cpp = Class.forName("com.skynet.codegenerator.cpp.CppCoreGenerator");
        Method fieldName = cpp.getDeclaredMethod("fieldName", FieldDescriptor.class);
        Method relationName = cpp.getDeclaredMethod("relationName", FieldDescriptor.class);
        Method camel = cpp.getDeclaredMethod("camel", String.class);
        Method descriptorFields = cpp.getDeclaredMethod("descriptorFields", ObjectDescriptor.class);
        Method typeName = cpp.getDeclaredMethod("typeName", ObjectDescriptor.class);
        fieldName.setAccessible(true);
        relationName.setAccessible(true);
        camel.setAccessible(true);
        descriptorFields.setAccessible(true);
        typeName.setAccessible(true);
        List<Map<String, Object>> aliases = new ArrayList<>();
        for (ObjectDescriptor entity : new EntityDomain(domain).getObjectDescriptors()) {
            @SuppressWarnings("unchecked")
            List<FieldDescriptor> projectionFields = (List<FieldDescriptor>) descriptorFields.invoke(null, entity);
            Map<String, String> identities = new LinkedHashMap<>();
            List<String> projection = new ArrayList<>();
            for (FieldDescriptor field : projectionFields) {
                String canonical = RustFieldDescriptorNaming.rustStorageFieldName(field);
                projection.add(canonical);
                identities.put(canonical, RustFieldDescriptorNaming.rustStorageMemberName(field));
            }
            for (FieldDescriptor field : entity.getFields()) {
                String member = (String) fieldName.invoke(null, field);
                String aggregateMember = field.isObjectProperty()
                        ? (String) relationName.invoke(null, field) : member;
                Map<String, Object> row = new LinkedHashMap<>();
                row.put("entity", entity.getName());
                row.put("cpp_type", typeName.invoke(null, entity));
                row.put("identities", identities);
                row.put("projection", projection);
                row.put("canonical", RustFieldDescriptorNaming.rustStorageFieldName(field));
                row.put("source_member", RustFieldDescriptorNaming.rustStorageMemberName(field));
                row.put("source_count_member", field.isObjectProperty()
                        ? RustFieldDescriptorNaming.rustMemberName(field)
                        : RustFieldDescriptorNaming.rustStorageMemberName(field));
                row.put("cpp_member", member);
                row.put("object", field.isObjectProperty());
                row.put("cpp_count_member", aggregateMember);
                Map<String, String> methods = new LinkedHashMap<>();
                for (String method : List.of("sum", "avg", "min", "max")) {
                    methods.put(method, (String) camel.invoke(null, method + "_" + member));
                }
                row.put("methods", methods);
                aliases.add(row);
            }
        }
        Files.writeString(Path.of(args[1]), JSONUtil.toJsonStr(Map.of("aliases", aliases)) + "\n");
    }
}
