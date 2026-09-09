package main.java;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.Map;
import java.util.stream.Gatherer;
import java.util.stream.Stream;

public class main {

    static void main(String[] args) throws IOException {
        String pathname = "generate/measurements1b.txt";
        if (args.length > 0) pathname = args[0];

        try (Stream<String> reader = Files.newBufferedReader(Path.of(pathname)).lines()) {
            System.out.println("running it");


            final var result = reader.parallel()
                                     .<Map<String, StationData>>gather(Gatherer.of(
                                             StationManager::new,
                                             (manager, line, down) -> {
                                                 manager.add(line);
                                                 return true;
                                             },  // combiner
                                             (mainInstance, worker) -> {
                                                 mainInstance.mergeFromOther(worker);
                                                 return mainInstance;
                                             },
                                             (sm, down) -> down.push(sm.stations)
                                     ));
            result.forEach(e -> e.forEach((_, stationData) -> System.out.println(stationData)));
        }catch (IOException e){
            e.printStackTrace();
        }
    }
}

class StationData {
    private final String name;
    float min;
    float max;
    float sum;
    int records = 0;


    public StationData(String name, float temp) {
        this.name = name;
        records = 1;
        min = temp;
        max = temp;
        sum = temp;
    }

    public StationData(String name) {
        this.name = name;
    }

    public void setData(float temp) {
        sum += temp;
        max = Math.max(max, temp);
        min = Math.min(min, temp);
        records++;
    }

    void mergeData(StationData o) {
        min = Math.min(min, o.min);
        max = Math.max(max, o.max);
        sum += o.sum;
        records += o.records;
    }

    @Override
    public String toString() {
        return "%s=%.1f/%.1f/%.1f".formatted(name, min, max, (sum / (float) records));
    }
}

class StationManager {

    final Map<String, StationData> stations = new HashMap<>();

    public StationManager() {

    }

    static float parse(String s) {
        int i = 0;
        int sign = 1;
//im on crack
        if (s.charAt(0) == '-') {
            sign = -1;
            i++;
        }

        int n = 0;
        for (; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c != '.') {
                n = n * 10 + (c - '0');
            }
        }

        return sign * n * 0.1f;
    }

    void mergeFromOther(StationManager manager) {
        manager.stations.forEach((name, stationData) ->
                                         stations.computeIfAbsent(name, StationData::new)
                                                 .mergeData(stationData));
    }

    public void add(String station) {
        final var stationSplit = station.split(";");
        final float tempf = parse(stationSplit[1]);

//                    stations.putIfAbsent(stationSplit[0], new StationData(stationSplit[0]));
        try {
            stations.computeIfAbsent(stationSplit[0], StationData::new)
                    .setData(tempf);
        } catch (Exception e) {
            System.out.println("station[0]: " +
                               stationSplit[0] +
                               " temp(str): " +
                               stationSplit[1] +
                               " tempf: " +
                               tempf +
                               " temp(parsed): " +
                               Float.parseFloat(stationSplit[1]));
            e.printStackTrace();

        }
    }
}