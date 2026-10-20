import com.skynet.codegenerator.*;
import com.skynet.codegenerator.meta.*;
import com.skynet.codegenerator.naming.RustFieldDescriptorNaming;
import cn.hutool.json.JSONUtil;
import java.lang.reflect.*;
import java.nio.file.*;
import java.util.*;

/** Read model fields, C++ keywords, type and method spellings from the real producer. */
public final class ScalarDiagnosticMetadataProbe {
    public static void main(String[] args) throws Exception {
        Path input = Path.of(args[0]);
        GenerateContext context = new GenerateContext("cpp-lib-core", input.getFileName().toString());
        context.setEntranceFile(input.getFileName().toString());
        context.appendFile(input.getFileName().toString(), Files.readAllBytes(input));
        BaseDomainManager manager = new BaseDomainManager();
        Domain domain = manager.getDomain(manager.parse(context));
        Class<?> cpp = Class.forName("com.skynet.codegenerator.cpp.CppCoreGenerator");
        Method fields = cpp.getDeclaredMethod("fields", ObjectDescriptor.class);
        Method fieldName = cpp.getDeclaredMethod("fieldName", FieldDescriptor.class);
        Method typeName = cpp.getDeclaredMethod("typeName", ObjectDescriptor.class);
        Method applicationType = cpp.getDeclaredMethod("applicationType", FieldDescriptor.class);
        Method camel = cpp.getDeclaredMethod("camel", String.class);
        Field keywordField = cpp.getDeclaredField("KEYWORDS");
        for (var method : List.of(fields, fieldName, typeName, applicationType, camel)) {
            method.setAccessible(true);
        }
        keywordField.setAccessible(true);
        Set<?> keywords = (Set<?>) keywordField.get(null);
        List<Map<String, Object>> rows = new ArrayList<>();
        for (ObjectDescriptor entity : new EntityDomain(domain).getObjectDescriptors()) {
            for (FieldDescriptor field : (List<FieldDescriptor>) fields.invoke(null, entity)) {
                String identity = RustFieldDescriptorNaming.rustStorageFieldName(field);
                if (field.isObjectProperty() || !keywords.contains(identity)) {
                    continue;
                }
                String member = (String) fieldName.invoke(null, field);
                Map<String, Object> row = new LinkedHashMap<>();
                row.put("entity", typeName.invoke(null, entity));
                row.put("identity", identity);
                row.put("type", applicationType.invoke(null, field));
                for (String operation : List.of("select", "unselect", "get")) {
                    row.put(operation, camel.invoke(null, operation + "_" + member));
                    row.put(operation + "_alias", camel.invoke(null, operation + "_" + identity));
                }
                rows.add(row);
            }
        }
        Files.writeString(Path.of(args[1]), JSONUtil.toJsonStr(rows) + "\n");
    }
}
