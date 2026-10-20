import com.skynet.codegenerator.*;
import com.skynet.codegenerator.meta.*;
import com.skynet.codegenerator.naming.*;
import cn.hutool.json.JSONUtil;
import java.lang.reflect.Method;
import java.nio.file.*;
import java.util.*;

/** Observe source relationship identities separately from native C++ identifiers. */
public final class RelationIdentityMetadataProbe {
    public static void main(String[] args) throws Exception {
        Path input = Path.of(args[0]);
        GenerateContext context = new GenerateContext("cpp-lib-core", input.getFileName().toString());
        context.setEntranceFile(input.getFileName().toString());
        context.appendFile(input.getFileName().toString(), Files.readAllBytes(input));
        BaseDomainManager manager = new BaseDomainManager();
        Domain domain = manager.getDomain(manager.parse(context));
        Class<?> cpp = Class.forName("com.skynet.codegenerator.cpp.CppCoreGenerator");
        Method forward = cpp.getDeclaredMethod("relationName", FieldDescriptor.class);
        Method reverse = cpp.getDeclaredMethod("relationName", ReverseRelation.class);
        Method camel = cpp.getDeclaredMethod("camel", String.class);
        Method type = cpp.getDeclaredMethod("typeName", ObjectDescriptor.class);
        for (Method method : List.of(forward, reverse, camel, type)) {
            method.setAccessible(true);
        }
        List<Map<String, Object>> rows = new ArrayList<>();
        for (ObjectDescriptor entity : new EntityDomain(domain).getObjectDescriptors()) {
            List<String> fields = new ArrayList<>();
            for (FieldDescriptor field : entity.getFields()) {
                fields.add(RustFieldDescriptorNaming.rustStorageFieldName(field));
                fields.add(FieldDescriptorNaming.columnName(field));
                if (field.isObjectProperty()) {
                    append(rows, entity, field, forward.invoke(null, field).toString(),
                           RustFieldDescriptorNaming.rustMemberName(field),
                           ((ObjectType) field.getFieldType()).getRefer(), true, false, fields, type, camel);
                }
            }
            List<String> descriptors = new ArrayList<>();
            for (ReverseRelation relation : RustObjectDescriptorNaming.rustOneToOneRelations(entity)) {
                descriptors.add(RustReverseRelationNaming.rustMemberName(relation));
            }
            for (ReverseRelation relation : RustObjectDescriptorNaming.rustOneToManyRelations(entity)) {
                descriptors.add(RustReverseRelationNaming.rustMemberName(relation));
            }
            for (ReverseRelation relation : entity.getReverseRelations()) {
                append(rows, entity, relation, reverse.invoke(null, relation).toString(),
                       RustReverseRelationNaming.rustMemberName(relation), relation.getChild(),
                       false, !ReverseRelationNaming.oneToOne(relation), fields, type, camel);
                rows.get(rows.size() - 1).put("descriptor_index", descriptors.indexOf(RustReverseRelationNaming.rustMemberName(relation)));
            }
        }
        Files.writeString(Path.of(args[1]), JSONUtil.toJsonStr(rows) + "\n");
    }

    private static void append(List<Map<String, Object>> rows, ObjectDescriptor entity,
                               Object relation, String member, String identity,
                               ObjectDescriptor target, boolean forward, boolean many,
                               List<String> fields, Method type, Method camel) throws Exception {
        Map<String, Object> row = new LinkedHashMap<>();
        row.put("entity", type.invoke(null, entity));
        row.put("target", type.invoke(null, target));
        row.put("member", member);
        row.put("identity", identity);
        row.put("forward", forward);
        row.put("many", many);
        row.put("fields", fields);
        row.put("accessor", camel.invoke(null, member));
        row.put("select", camel.invoke(null, "select_" + member));
        row.put("select_alias", camel.invoke(null, "select_" + identity));
        row.put("unselect", camel.invoke(null, "unselect_" + member));
        row.put("unselect_alias", camel.invoke(null, "unselect_" + identity));
        row.put("get", camel.invoke(null, "get_" + member));
        row.put("get_alias", camel.invoke(null, "get_" + identity));
        rows.add(row);
    }
}
