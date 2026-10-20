import com.skynet.codegenerator.*;
import com.skynet.codegenerator.meta.*;
import com.skynet.codegenerator.naming.*;
import cn.hutool.json.JSONUtil;
import java.lang.reflect.*;
import java.nio.file.*;
import java.util.*;

/** Observe actual own-field lookup eligibility and centralized C++/Rust naming. */
public final class BusinessIdLookupMetadataProbe {
    public static void main(String[] args) throws Exception {
        Path input=Path.of(args[0]);
        GenerateContext context=new GenerateContext("cpp-lib-core",input.getFileName().toString());
        context.setEntranceFile(input.getFileName().toString());
        context.appendFile(input.getFileName().toString(),Files.readAllBytes(input));
        BaseDomainManager manager=new BaseDomainManager();
        Domain domain=manager.getDomain(manager.parse(context));
        Class<?> cpp=Class.forName("com.skynet.codegenerator.cpp.CppCoreGenerator");
        Method type=cpp.getDeclaredMethod("typeName",ObjectDescriptor.class);
        Method application=cpp.getDeclaredMethod("applicationType",FieldDescriptor.class);
        Method identifier=cpp.getDeclaredMethod("identifier",String.class);
        Method canonical=cpp.getDeclaredMethod("canonical",FieldDescriptor.class);
        Method camel=cpp.getDeclaredMethod("camel",String.class);
        Method isBusinessId=Class.forName("com.skynet.codegenerator.cpp.CppBusinessIdMetadata")
            .getDeclaredMethod("isBusinessId",FieldDescriptor.class);
        isBusinessId.setAccessible(true);
        for (Method method:List.of(type,application,identifier,canonical,camel)) {method.setAccessible(true);}
        List<Map<String,Object>> rows=new ArrayList<>();
        for (ObjectDescriptor entity:new EntityDomain(domain).getObjectDescriptors()) {
            for (FieldDescriptor field:entity.getFields()) {
                if (!(boolean)isBusinessId.invoke(null,field)) {continue;}
                Map<String,Object> row=new LinkedHashMap<>();
                row.put("entity",type.invoke(null,entity));
                row.put("field",canonical.invoke(null,field));
                row.put("type",application.invoke(null,field));
                row.put("lookup",camel.invoke(null,identifier.invoke(null,
                    entity.getName()+"_by_"+canonical.invoke(null,field))));
                row.put("entry",camel.invoke(null,identifier.invoke(null,PluralizationUtils.pluralize(entity.getName()))));
                row.put("predicate",camel.invoke(null,identifier.invoke(null,
                    FieldDescriptorNaming.attributePredicatePrefixExpr(field))+"_is"));
                row.put("rust_lookup",RustObjectDescriptorNaming.rustModuleName(entity)+"_by_"
                    +RustFieldDescriptorNaming.rustStorageMemberName(field));
                rows.add(row);
            }
        }
        Files.writeString(Path.of(args[1]),JSONUtil.toJsonStr(rows)+"\n");
    }
}
