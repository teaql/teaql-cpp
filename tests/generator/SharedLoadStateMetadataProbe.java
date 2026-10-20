import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.skynet.codegenerator.meta.*;
import com.skynet.codegenerator.naming.*;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.file.*;
import java.util.*;

/** Calls actual numbering/revision implementations on explicit descriptor inputs. */
public final class SharedLoadStateMetadataProbe {
    private static final ObjectMapper JSON = new ObjectMapper();

    public static void main(String[] args) throws Exception {
        boolean original = args[0].equals("reference");
        List<Map<String, Object>> observations = new ArrayList<>();
        for (JsonNode input : JSON.readTree(Files.readAllBytes(Path.of(args[1])))) {
            observations.add(observe(input, original));
        }
        Files.writeString(Path.of(args[2]), JSON.writeValueAsString(observations) + "\n");
    }

    private static String nullableText(JsonNode input, String key) {
        JsonNode value = input.get(key);
        return value == null || value.isNull() ? null : value.asText();
    }

    private static Map<String, Object> observe(JsonNode input, boolean original) {
        Map<String, Object> result = new LinkedHashMap<>();
        try {
            Domain domain = new Domain();
            domain.setName(nullableText(input, "domain"));
            for (JsonNode item : input.get("entities")) {
                ObjectDescriptor entity = new ObjectDescriptor();
                entity.setName(nullableText(item, "name"));
                entity.setDomain(item.path("null_domain").asBoolean() ? null : domain);
                if (item.has("parent")) {
                    entity.appendAttribute(Attributes.PARENT, nullableText(item, "parent"));
                }
                for (JsonNode itemField : item.get("fields")) {
                    FieldDescriptor field = new FieldDescriptor();
                    field.setName(nullableText(itemField, "name"));
                    field.setType(nullableText(itemField, "type"));
                    field.setEntityType(nullableText(itemField, "entity_type"));
                    field.setObjectDescriptor(entity);
                    for (String attribute : List.of(Attributes.SQL_COLUMN, Attributes.SQL_TABLE)) {
                        if (itemField.has(attribute)) {
                            field.appendAttribute(attribute, nullableText(itemField, attribute));
                        }
                    }
                    // Explicit physical names keep this a descriptor API observer,
                    // independent of provider bootstrap or normalized-model parsing.
                    if (!itemField.has(Attributes.SQL_COLUMN)) {
                        field.appendAttribute(Attributes.SQL_COLUMN, field.getName());
                    }
                    if (itemField.path("object").asBoolean()) {
                        ObjectDescriptor referenced = new ObjectDescriptor();
                        referenced.setName(field.getEntityType() == null ? "Target" : field.getEntityType());
                        referenced.setDomain(domain);
                        ObjectType type = new ObjectType();
                        type.setRefer(referenced);
                        field.setFieldType(type);
                    }
                    entity.getFields().add(field);
                }
                domain.getObjectDescriptors().add(entity);
            }
            ObjectDescriptor entity = domain.findObjectDescriptor(input.get("target").asText());
            List<?> fields;
            String revision;
            if (original) {
                fields = (List<?>) ObjectDescriptor.class.getMethod("getPredefinedFieldIndexes")
                    .invoke(entity);
                revision = ObjectDescriptor.class.getMethod("getPredefinedFieldLayoutRevision")
                    .invoke(entity).toString();
            } else {
                Class<?> metadata = Class.forName("com.skynet.codegenerator.cpp.CppFieldLayoutMetadata");
                Method collect = metadata.getDeclaredMethod("fields", ObjectDescriptor.class);
                Method fingerprint = metadata.getDeclaredMethod("revision", ObjectDescriptor.class);
                collect.setAccessible(true);
                fingerprint.setAccessible(true);
                fields = (List<?>) collect.invoke(null, entity);
                revision = fingerprint.invoke(null, entity).toString();
            }
            List<Map<String, Object>> entries = new ArrayList<>();
            for (int i = 0; i < fields.size(); ++i) {
                Object item = fields.get(i);
                Map<String, Object> entry = new LinkedHashMap<>();
                FieldDescriptor field;
                if (original) {
                    Class<?> index = item.getClass();
                    field = (FieldDescriptor) index.getMethod("getField").invoke(item);
                    entry.put("index", index.getMethod("getIndex").invoke(item));
                    entry.put("canonical", index.getMethod("getCanonicalName").invoke(item));
                    entry.put("member", index.getMethod("getJavaMemberName").invoke(item));
                    entry.put("storage", index.getMethod("getRustMemberName").invoke(item));
                    entry.put("column", index.getMethod("getColumnName").invoke(item));
                } else {
                    field = (FieldDescriptor) item;
                    // CppCoreGenerator.emitFieldLayout emits the list position.
                    entry.put("index", i);
                    entry.put("canonical", FieldDescriptorNaming.canonicalPropertyName(field));
                    entry.put("member", FieldDescriptorNaming.memberName(field));
                    entry.put("storage", RustFieldDescriptorNaming.rustStorageMemberName(field));
                    entry.put("column", FieldDescriptorNaming.columnName(field));
                }
                entry.put("owner", field.getObjectDescriptor().getName());
                entry.put("type", field.getType());
                entry.put("entity_type", field.getEntityType());
                entries.add(entry);
            }
            result.put("ok", true);
            result.put("fields", entries);
            result.put("revision", revision);
        } catch (Throwable failure) {
            while (failure instanceof InvocationTargetException && failure.getCause() != null) {
                failure = failure.getCause();
            }
            result.put("ok", false);
            result.put("class", failure.getClass().getName());
            result.put("message", failure.getMessage());
        }
        return result;
    }
}
