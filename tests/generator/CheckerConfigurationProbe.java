import com.skynet.codegenerator.*;
import com.skynet.codegenerator.meta.*;
import cn.hutool.json.JSONUtil;
import java.nio.file.*;
import java.util.*;

/** Observe actual model selection and inherited Checker metadata, not source counts. */
public final class CheckerConfigurationProbe {
    public static void main(String[] args) throws Exception {
        Path input = Path.of(args[0]), output = Path.of(args[1]);
        GenerateContext context = new GenerateContext("rust-lib-core", input.getFileName().toString());
        context.setEntranceFile(input.getFileName().toString());
        context.appendFile(input.getFileName().toString(), Files.readAllBytes(input));
        BaseDomainManager manager = new BaseDomainManager();
        Domain domain = manager.getDomain(manager.parse(context));
        Map<String, Object> result = new LinkedHashMap<>();
        List<Map<String, Object>> models = new ArrayList<>();
        for (ObjectDescriptor entity : domain.getObjectDescriptors()) {
            Map<String, Object> model = new LinkedHashMap<>();
            model.put("name", entity.getName());
            model.put("view", entity.getBooleanAttribute(Attributes.ATTRIBUTE_VIEW));
            model.put("value_object", entity.getBooleanAttribute(Attributes.ATTRIBUTE_VALUE_OBJECT));
            model.put("parent", entity.getParentOd() == null ? null : entity.getParentOd().getName());
            List<Map<String, Object>> fields = new ArrayList<>();
            for (FieldDescriptor field : entity.getFields()) {
                Map<String, Object> item = new LinkedHashMap<>();
                item.put("name", field.getName());
                item.put("context_root", field.isContextRoot());
                for (String attribute : List.of("contextRoot", "createFunction", "updateFunction", "optional")) {
                    item.put(attribute, field.getAttributeValue(attribute));
                }
                fields.add(item);
            }
            model.put("fields", fields);
            models.add(model);
        }
        result.put("models", models);
        result.put("selected_entities", new EntityDomain(domain).getObjectDescriptors().stream()
            .map(ObjectDescriptor::getName).toList());
        Files.writeString(output, JSONUtil.toJsonStr(result) + "\n");
    }
}
