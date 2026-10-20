import com.skynet.codegenerator.GenerateContext;
import com.skynet.codegenerator.GeneratedFile;
import com.skynet.codegenerator.meta.BaseDomainManager;
import com.skynet.codegenerator.meta.Domain;
import com.skynet.codegenerator.rust.RustWorkspaceI18nGenerator;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

/** Invoke the actual locked Rust workspace catalog generator, without an application SDK. */
public final class I18nCatalogProbe {
    public static void main(String[] args) throws Exception {
        Path input = Path.of(args[0]);
        GenerateContext context = new GenerateContext("rust-app-console", input.getFileName().toString());
        context.setEntranceFile(input.getFileName().toString());
        context.appendFile(input.getFileName().toString(), Files.readAllBytes(input));
        BaseDomainManager manager = new BaseDomainManager();
        Domain domain = manager.getDomain(manager.parse(context));
        List<GeneratedFile> files = new RustWorkspaceI18nGenerator().generate(domain);
        if (files.size() != 1 || !files.get(0).getPath().equals("teaql-i18n.json")) {
            throw new IllegalStateException("Unexpected locked workspace catalog output");
        }
        Files.write(Path.of(args[1]), files.get(0).getData());
    }
}
